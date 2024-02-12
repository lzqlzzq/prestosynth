

namespace psynth {

class LowPassFilter {
private:
	float a1, a2, a3, a4;
public:
	LowPassFilter(uint16_t cutOffFreq, // In Hz
		uint16_t sampleRate,  // In Hz
		float Q);  // In Db

	void process(std::vector<float> &signal);
};

}