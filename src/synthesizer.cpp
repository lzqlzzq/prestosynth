#include "prestosynth/synthesizer.h"

#include <algorithm>
#include <thread>
#include <mutex>

#include "prestosynth/util/math_util.h"

namespace psynth {

bool NoteHead::operator<(const NoteHead &other) const {
    return (this->duration < other.duration) ||
            (this->duration == other.duration && this->pitch < other.pitch) ||
            (this->duration == other.duration && this->pitch < other.pitch && this->velocity < other.velocity);
};

Synthesizer::Synthesizer(const std::string &sfPath, uint32_t sampleRate, uint8_t quality, uint8_t workerNum):
    sf(PrestoSoundFont(sfPath, sampleRate, quality)), sampleRate(sampleRate), workerNum(workerNum) {};

NoteMap Synthesizer::map_notes(const Notes &notes) {
    NoteMap noteMap;
    for(const Note &note : notes) {
        uint32_t startFrame = s_to_frames(note.start, sampleRate);
        uint32_t durationFrame = s_to_frames(note.duration, sampleRate);

        NoteHead head = {durationFrame, note.pitch, note.velocity};

        if(!noteMap.count(head))
            noteMap[head] = NoteStartPack();

        noteMap[head].emplace_back(startFrame);
    }
    return noteMap;
};

AudioData Synthesizer::render_single_thread(const Track &track, bool stereo) {
    AudioData trackAudio = Eigen::ArrayXXf::Zero(stereo ? 2 : 1, s_to_frames(track.end(), sampleRate));

    for(const auto &pack : map_notes(track.notes)) {
        const auto &head = pack.first;
        AudioData noteAudio = sf.build_note(
            track.preset,
            track.bank,
            head.pitch,
            head.velocity,
            head.duration,
            stereo);

        for(uint32_t startFrame : pack.second) {
            if(startFrame + noteAudio.cols() > trackAudio.cols())
                trackAudio.conservativeResize(Eigen::NoChange, startFrame + noteAudio.cols());

            trackAudio.middleCols(startFrame, noteAudio.cols()) += noteAudio;
        }
    }

    return trackAudio * db_to_amplitude(track.volume);
};

AudioData Synthesizer::render_multi_thread(const Track &track, bool stereo) {
    if(!track.notes.size())
        return Eigen::ArrayXXf::Zero(stereo ? 2 : 1, 1);

    NoteMap noteMap = map_notes(track.notes);

    PackedNoteQueue noteQueue(noteMap.size());
    for(const auto &pack : map_notes(track.notes)) {
        noteQueue.push(PackedNote{pack.first, pack.second});
    }
    
    NoteAudioQueue audioQueue(noteMap.size());
    std::mutex condMtx;
    uint8_t aliveWorkerNum = workerNum;
    std::vector<std::thread> workers;
    for(int i = 0; i < workerNum; i++) {
        workers.emplace_back([&] {
            while(!noteQueue.empty()) {
                PackedNote pack;
                if(noteQueue.try_pop(pack)) {
                    const auto &head = pack.head;
                    AudioData noteAudio = sf.build_note(
                        track.preset,
                        track.bank,
                        head.pitch,
                        head.velocity,
                        head.duration,
                        stereo);
                    audioQueue.push(NoteAudioPack{noteAudio, pack.startPack});
                }
            }
            condMtx.lock();
            aliveWorkerNum--;
            condMtx.unlock();
        });
    }

    AudioData trackAudio = Eigen::ArrayXXf::Zero(stereo ? 2 : 1, s_to_frames(track.end(), sampleRate));
    while(!audioQueue.empty() || aliveWorkerNum) {
        NoteAudioPack pack;
        if(audioQueue.try_pop(pack)) {
            AudioData &noteAudio = pack.audio;
            for(uint32_t startFrame : pack.startPack) {
                if(startFrame + noteAudio.cols() > trackAudio.cols())
                    trackAudio.conservativeResize(Eigen::NoChange, startFrame + noteAudio.cols());

                trackAudio.middleCols(startFrame, noteAudio.cols()) += noteAudio;
            }
        }
    }

    for(int i = 0; i < workerNum; i++) {
        workers[i].join();
    }

    return trackAudio * db_to_amplitude(track.volume);
};

AudioData Synthesizer::render(const Track &track, bool stereo) {
    if(workerNum < 2)
        return render_single_thread(track, stereo);
    else
        return render_multi_thread(track, stereo);
};

AudioData Synthesizer::render(const Sequence &sequence, bool stereo) {
    AudioData master = Eigen::ArrayXXf::Zero(stereo ? 2 : 1, 1);
    for(const Track &track : sequence.tracks) {
        AudioData trackAudio = (render(track, stereo));

        if(master.cols() < trackAudio.cols())
            std::swap(master, trackAudio);

        master.leftCols(trackAudio.cols()) += trackAudio;
    }

    return master * db_to_amplitude(sequence.volume);
};

}
