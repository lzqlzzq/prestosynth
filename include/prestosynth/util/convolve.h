#ifndef _CONVOLVE_H
#define _CONVOLVE_H

#include <iostream>
#include "prestosynth/util/audio_util.h"

namespace psynth {

template<size_t N>
using Kernel = Eigen::Matrix<float, N, 1>;

template<typename Kernel, size_t N = Kernel::SizeAtCompileTime>
void conv1d(AudioData &signal, const Kernel &kernel) {
	for(int r = 0; r < signal.rows(); ++r) {
		for(int c = signal.cols() - 1; c > N - 1; --c) {
			signal.row(r).col(c) = signal.row(r).segment(c - (N - 1), N).matrix() * kernel;
		}

		// Left padding
		for(int c = N - 1; c >= 0; --c) {
			signal.row(r).col(c) = signal.row(r).head(c + 1).matrix() * kernel.tail(c + 1);
		}
	}
};

}

#endif
