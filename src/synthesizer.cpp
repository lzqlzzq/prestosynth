#include "prestosynth/synthesizer.h"

#include <algorithm>
#include <thread>
#include <mutex>
#include <random>

#include "prestosynth/util/math_util.h"

namespace psynth {

static std::default_random_engine randomEngine(114514);

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

    for(auto &item : noteMap) {
        std::shuffle(item.second.begin(), item.second.begin(), randomEngine);
    }

    return noteMap;
};

AudioData Synthesizer::render_single_thread(const Track &track, bool stereo) {
    AudioData trackAudio = Eigen::ArrayXXf::Zero(stereo ? 2 : 1, s_to_frames(track.end(), sampleRate));
    float volume = db_to_amplitude(track.volume);

    for(const auto &pack : map_notes(track.notes)) {
        const auto &head = pack.first;
        AudioData noteAudio = sf.build_note(
            track.preset,
            track.bank,
            head.pitch,
            head.velocity,
            head.duration,
            stereo);
        if(stereo) {
            noteAudio.row(0) *= volume * (0.5 - track.pan);
            noteAudio.row(1) *= volume * (0.5 + track.pan);
        } else 
            noteAudio *=  volume;

        for(uint32_t startFrame : pack.second) {
            if(startFrame + noteAudio.cols() > trackAudio.cols())
                trackAudio.conservativeResize(Eigen::NoChange, startFrame + noteAudio.cols());

            trackAudio.middleCols(startFrame, noteAudio.cols()) += noteAudio;
        }
    }

    return trackAudio;
};

AudioData Synthesizer::render_multi_thread(const Track &track, bool stereo) {
    NoteMap noteMap = map_notes(track.notes);
    float volume = db_to_amplitude(track.volume);

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
                    if(stereo) {
                        noteAudio.row(0) *= volume * (0.5 - track.pan);
                        noteAudio.row(1) *= volume * (0.5 + track.pan);
                    } else 
                        noteAudio *=  volume;

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
        audioQueue.pop(pack);

        AudioData &noteAudio = pack.audio;
        for(uint32_t startFrame : pack.startPack) {
            if(startFrame + noteAudio.cols() > trackAudio.cols())
                trackAudio.conservativeResize(Eigen::NoChange, startFrame + noteAudio.cols());

            trackAudio.middleCols(startFrame, noteAudio.cols()) += noteAudio;
        }
    }

    for(int i = 0; i < workerNum; i++) {
        workers[i].join();
    }

    return trackAudio;
};

AudioData Synthesizer::render(const Track &track, bool stereo) {
    return render_single_thread(track, stereo);
};

AudioData Synthesizer::render_multi_thread(const Sequence &sequence, bool stereo) {
    AudioQueue audioQueue(sequence.tracks.size());
    std::mutex condMtx;
    std::mutex trackMtx;
    uint8_t trackIdx = 0;
    uint8_t aliveWorkerNum = workerNum;
    std::vector<std::thread> workers;
    for(int i = 0; i < workerNum; i++) {
        workers.emplace_back([&] {
            while(trackIdx != sequence.tracks.size()) {
                Track track;
                {
                    std::lock_guard<std::mutex> lk(trackMtx);
                    if(trackIdx == sequence.tracks.size())
                        break;

                    track = sequence.tracks[trackIdx];
                    trackIdx++;
                }
                if(!track.notes.size())
                    continue;

                audioQueue.push(render_single_thread(track, stereo));
            }
            std::lock_guard<std::mutex> lk(condMtx);
            aliveWorkerNum--;
        });
    }

    AudioData master = Eigen::ArrayXXf::Zero(stereo ? 2 : 1, 1);
    while(!audioQueue.empty() || aliveWorkerNum) {
        AudioData trackAudio;

        audioQueue.pop(trackAudio);
        if(master.cols() < trackAudio.cols())
            std::swap(master, trackAudio);

        master.leftCols(trackAudio.cols()) += trackAudio;
    }

    for(int i = 0; i < workerNum; i++) {
        workers[i].join();
    }

    return master * db_to_amplitude(sequence.volume);
}

AudioData Synthesizer::render_single_thread(const Sequence &sequence, bool stereo) {
    AudioData master = Eigen::ArrayXXf::Zero(stereo ? 2 : 1, s_to_frames(sequence.end(), sampleRate));
    for(const Track &track : sequence.tracks) {
        if(!track.notes.size())
            continue;

        float volume = db_to_amplitude(sequence.volume + track.volume);

        for(const auto &pack : map_notes(track.notes)) {
            const auto &head = pack.first;
            AudioData noteAudio = sf.build_note(
                track.preset,
                track.bank,
                head.pitch,
                head.velocity,
                head.duration,
                stereo);
            if(stereo) {
                noteAudio.row(0) *= volume * (0.5 - track.pan);
                noteAudio.row(1) *= volume * (0.5 + track.pan);
            } else 
                noteAudio *=  volume;

            for(uint32_t startFrame : pack.second) {
                if(startFrame + noteAudio.cols() > master.cols())
                    master.conservativeResize(Eigen::NoChange, startFrame + noteAudio.cols());

                master.middleCols(startFrame, noteAudio.cols()) += noteAudio;
            }
        }
    }

    return master;
};

AudioData Synthesizer::render(const Sequence &sequence, bool stereo) {
    if(sequence.tracks.size() > 1 && workerNum > 1)
        return render_multi_thread(sequence, stereo);
    else
        return render_single_thread(sequence, stereo);
};

}
