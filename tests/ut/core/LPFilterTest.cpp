/*
 *  Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#if !defined(LOG_THRESHOLD)
	// custom logging threshold - keep undefined on the repo
	// #define LOG_THRESHOLD   LOG_LEVEL_TRACE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <gtest/gtest.h>
#include "LPFilterTest.h"
#include "common/MathExt.h"
#include "core/Earth.h"
#include "common/Logger.h"
#include "BipedalAcceleration.h"

namespace fl {
namespace core {

constexpr   Hertz LPFilterTest::MIN_SAMPLING_FREQUENCY;
constexpr   Hertz LPFilterTest::MAX_SAMPLING_FREQUENCY;
constexpr   Hertz LPFilterTest::MIN_PROBE_FREQUENCY;
constexpr Seconds LPFilterTest::TEST_INTERVAL;

TEST_F(LPFilterTest, PureWave)
{
	Result result;

	LOG_TRACE(ENTER_FUNCTION_THIS_PREFIX("()"));
	for (Hertz fs = MIN_SAMPLING_FREQUENCY;  fs <= MAX_SAMPLING_FREQUENCY;  fs *= 2) {
		result = _lpFilter.reset(fs);
		EXPECT_EQ(result, Result::OK);
		if (result == Result::OK) {
			Seconds testInterval = fs * TEST_INTERVAL;
			for (Hertz fp = MIN_PROBE_FREQUENCY;  2 * fp <= fs;  fp *= 2) {
				real sum_u2, sum_v2;
				double omega;
				double t0;

				_lpFilter.resetMemory();
				sum_u2 = 0;
				sum_v2 = 0;
				omega = fp * (2 * M_PI) / fs;
				t0 = rand() * (1.0/(double(RAND_MAX) + 1));
				for (Seconds t = 0;  t < testInterval;  t += 1) {
					real u;

					// the test signal is pure wave with random phase
					u = static_cast<real>(sin((t0 + t)*omega));
					// collect i/o for comparison
					sum_u2 += square(u);
					sum_v2 += square(_lpFilter.process(u));
				}

				real G  = sum_v2 / sum_u2;
				real dB = 3 * log2(G);
				if (fp <= LPFilter::CUTOFF_FREQUENCY / 2)
					EXPECT_LE(square(dB), 1);
				else if (fp >= LPFilter::CUTOFF_FREQUENCY * 2)
					EXPECT_LE(dB, -12);
				LOG_INFO("fs=%5.1f Hz  fp=%6.3f Hz  G=%e (% 6.2f dB)", fs, fp, G, dB);
			}
		}
	}
	LOG_TRACE(LEAVE_FUNCTION_THIS_PREFIX("()"));
} // fl::core::LPFilterTest::PureWave


TEST_F(LPFilterTest, Acceleration)
{
	BipedalAcceleration walkingAcceleration(BipedalAcceleration::WALK);
	             Result result;

	LOG_TRACE(ENTER_FUNCTION_THIS_PREFIX("()"));
	for (Hertz fs = MIN_SAMPLING_FREQUENCY;  fs <= MAX_SAMPLING_FREQUENCY;  fs *= 2) {
		result = _lpFilter.reset(fs);
		EXPECT_EQ(result, Result::OK);
		if (result == Result::OK) {
			Seconds testInterval = fs * TEST_INTERVAL;
			for (Hertz fp = MIN_PROBE_FREQUENCY;  2 * fp <= fs;  fp *= 2) {
				real sum_u2, sum_v2;
				real omega;

				_lpFilter.resetMemory();
				sum_u2 = 0;
				sum_v2 = 0;
				omega = fp * static_cast<real>(2 * M_PI) / fs;
				for (Seconds t = 0;  t < testInterval;  ++t) {
					real r;

					// the test signal is squared gravity plus a 'walking' wave with
					// pi/2-phase to minimize aliasing effect at the edge of frequency domain.
					r = walkingAcceleration.get(M_PI / 2 + t * omega);
					// collect i/o for comparison
					real u = square(r) - square(earth::GRAVITY);
					sum_u2 += square(u);
					sum_v2 += square(_lpFilter.process(u));
				}
				real G = sum_v2/sum_u2;
				real dB = 3 * log2(G);
				if (fp <= LPFilter::CUTOFF_FREQUENCY / 2)
					EXPECT_LE(square(dB), 1);
				else if (fp >= LPFilter::CUTOFF_FREQUENCY * 2)
					EXPECT_LE(dB, -7);
				LOG_INFO("fs=%5.1f Hz  fp=%6.3f Hz  G=%e (% 6.2f dB)", fs, fp, G, dB);
			}
		}
	}
	LOG_TRACE(LEAVE_FUNCTION_THIS_PREFIX("()"));
} // fl::core::LPFilterTest::Acceleration

} // fl::core

} // fl
