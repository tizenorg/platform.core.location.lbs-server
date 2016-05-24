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

namespace fl {
namespace core {

TEST_F(LPFilterTest, PureWave)
{
	Result result;

	LOG_TRACE(ENTER_FUNCTION_THIS_PREFIX("()"));
	for (unsigned fs = MIN_SAMPLING_FREQUENCY_mHz;  fs <= MAX_SAMPLING_FREQUENCY_mHz;  fs *= 2) {
		result = _lpFilter.reset(fs*static_cast<real>(1.0 / FREQUENCY_UNIT));
		EXPECT_EQ(result, Result::OK);
		if (result == Result::OK) {
			unsigned testInterval = fs * TEST_INTERVAL_s/FREQUENCY_UNIT;
			for (unsigned fp = MIN_PROBE_FREQUENCY_mHz;  2 * fp <= fs;  fp *= 2) {
				real sum_u2, sum_v2;
				real omega;
				real t0;

				_lpFilter.resetMemory();
				sum_u2 = 0;
				sum_v2 = 0;
				omega = fp * static_cast<real>(2 * M_PI) / fs;
				t0 = rand() * static_cast<real>(1.0/(RAND_MAX + 1));
				for (unsigned t = 0;  t < testInterval;  ++t) {
					real u;

					// the test signal is pure wave with random phase
					u = sin((t0 + t)*omega);
					// collect i/o for comparison
					sum_u2 += square(u);
					sum_v2 += square(_lpFilter.process(u));
				}

				real G  = sum_v2 / sum_u2;
				real dB = 3 * log2(G);
				if (fp <= LPFilter::CUTOFF_FREQUENCY / 2 * FREQUENCY_UNIT)
					EXPECT_LE(square(dB), 1);
				else if (fp >= LPFilter::CUTOFF_FREQUENCY * 2 * FREQUENCY_UNIT)
					EXPECT_LE(dB, -12);
				LOG_INFO("fs=%5.1f Hz  fp=%6.3f Hz  G=%e (% 6.2f dB)", fs * (1.0 / FREQUENCY_UNIT), fp * (1.0/FREQUENCY_UNIT), G, dB);
			}
		}
	}
	LOG_TRACE(LEAVE_FUNCTION_THIS_PREFIX("()"));
} // fl::core::LPFilterTest::PureWave

TEST_F(LPFilterTest, Acceleration)
{
	Result result;

	LOG_TRACE(ENTER_FUNCTION_THIS_PREFIX("()"));
	for (unsigned fs = MIN_SAMPLING_FREQUENCY_mHz;  fs <= MAX_SAMPLING_FREQUENCY_mHz;  fs *= 2) {
		result = _lpFilter.reset(fs * static_cast<real>(1.0 / FREQUENCY_UNIT));
		EXPECT_EQ(result, Result::OK);
		if (result == Result::OK) {
			unsigned testInterval = fs*TEST_INTERVAL_s/FREQUENCY_UNIT;
			for (unsigned fp = MIN_PROBE_FREQUENCY_mHz;  2 * fp <= fs;  fp *= 2) {
				real sum_u2, sum_v2;
				real omega;
				real t0;

				_lpFilter.resetMemory();
				sum_u2 = 0;
				sum_v2 = 0;
				omega = fp * static_cast<real>(2 * M_PI) / fs;
				t0 = rand() * static_cast<real>(1.0 / (RAND_MAX + 1));
				for (unsigned t = 0;  t < testInterval;  ++t) {
					real r;

					// the test signal is squared gravity plus pure wave with random phase
					r = earth::GRAVITY + sin((t0 + t) * omega);
					// collect i/o for comparison
					real u = square(r) - square(earth::GRAVITY);
					sum_u2 += square(u);
					sum_v2 += square(_lpFilter.process(u));
				}
				real G = sum_v2/sum_u2;
				real dB = 3 * log2(G);
				if (fp <= LPFilter::CUTOFF_FREQUENCY / 2 * FREQUENCY_UNIT)
					EXPECT_LE(square(dB), 1);
				else if (fp >= LPFilter::CUTOFF_FREQUENCY * 2 * FREQUENCY_UNIT)
					EXPECT_LE(dB, -12);
				LOG_INFO("fs=%5.1f Hz  fp=%6.3f Hz  G=%e (% 6.2f dB)", fs * (1.0 / FREQUENCY_UNIT), fp * (1.0 / FREQUENCY_UNIT), G, dB);
			}
		}
	}
	LOG_TRACE(LEAVE_FUNCTION_THIS_PREFIX("()"));
} // fl::core::LPFilterTest::Acceleration

} // fl::core

} // fl
