/*
 *	Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#if !defined(LOG_THRESHOLD)
	// custom logging threshold - keep undefined on the repo
	// #define LOG_THRESHOLD   LOG_LEVEL_TRACE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <gtest/gtest.h>
#include "MotionDetectorTest.h"
#include "common/MathExt.h"
#include "common/Logger.h"

#define LOG_PRECISION           real(0.1)   // maximal log(a/b)^2
#define MDF_TEST_START_TIME     real(3.14)  // [s] anything greater than zero
#define MDF_TEST_INTERVAL       real(100.0) // [s]
#define MDF_SAMPLING_INTERVAL   real(1.0 / MDF_SAMPLING_FREQUENCY)

namespace fl {
namespace core {

void MotionDetectorTest::onSignificantMotionDetected(void)
{
	LOG_TRACE(FUNCTION_THIS_PREFIX("(), __receivedTriggers:%u->%u"), __receivedTriggers, __receivedTriggers + 1);
	__receivedTriggers++;
} // fl::core::MotionDetectorTest::onSignificantMotionDetected

/*******************************************************************//**
 *	Simplest test with no measurement uncertainty, and no acceleration:
 *	expecting sigma2 = 0 all the time.
 */
TEST_F(MotionDetectorTest, Idle)
{
	AccelerationVector av;

	av[SPATIAL_X] = 0;
	av[SPATIAL_Y] = 0;
	av[SPATIAL_Z] = -LPF_GRAVITY;
	// we do not expect any events
	__receivedTriggers = 0;
	LOG_TRACE(ENTER_FUNCTION_THIS_PREFIX("()"));
	for (real dt = 0;  dt < MDF_TEST_INTERVAL;  dt += MDF_SAMPLING_INTERVAL)
		onAccelerationData(MDF_TEST_START_TIME + dt, av);
	EXPECT_EQ(__receivedTriggers, 0);
	LOG_TRACE(LEAVE_FUNCTION_THIS_PREFIX("()"));
} // fl::core::MotionDetectorTest::Idle

/*******************************************************************//**
 *	Null measurement uncertainty (R=0), and constant acceleration. The
 *	predicted covariance matrix is compared to independently calculated
 *	one for the special case of a=const. Checking for disparity and
 *	symmetry.
 */
TEST_F(MotionDetectorTest, ConstAcceleration)
{
	              real a, s2;
	              real expectedSigma2;
	AccelerationVector av;
	              real triggerTime;
	          unsigned receivedTriggers;

	av[SPATIAL_X] = 0;
	av[SPATIAL_Y] = 0;
	av[SPATIAL_Z] = -LPF_GRAVITY;

	LOG_TRACE(ENTER_FUNCTION_THIS_PREFIX("()"));

	a = LPF_GRAVITY + 2 * real(rand()) / RAND_MAX - 1;
	s2 = (square(a) - SQUARE(LPF_GRAVITY)) / 3;
	// reset the filter to the input state in order to avoid transients
	__lpAccelFilter.resetMemory(s2);
	// also fool itself into believing in stationary state
	__s2a1 = s2;
	receivedTriggers = 0;
	triggerTime = 0;
	for (real dt = 0;  dt < MDF_TEST_INTERVAL;  dt += MDF_SAMPLING_INTERVAL) {
		real tau = dt - triggerTime;
		expectedSigma2 = SQUARE(MDF_POSITION_SIGMA) + square(tau) * SQUARE(MDF_VELOCITY_SIGMA) + s2 * cube(tau) / 3;
		av[SPATIAL_Z] = -a;
		onAccelerationData(MDF_TEST_START_TIME + dt, av);
		if (dt > 0 && receivedTriggers == __receivedTriggers) {
			// check the estimate
			real log_dif = square(log(__sigma2/expectedSigma2));
			if (log_dif > LOG_PRECISION)
				LOG_ERROR("sigma^2=%g, <sigma^2>=%g", __sigma2, expectedSigma2);
			EXPECT_LE(log_dif, LOG_PRECISION);
		} else {
			receivedTriggers = __receivedTriggers;
			triggerTime = dt;
		}
	}
	if (expectedSigma2 >= SQUARE(MDF_UNCERTAINTY_THRESHOLD))
		EXPECT_NE(__receivedTriggers, 0);
	LOG_TRACE(LEAVE_FUNCTION_THIS_PREFIX("(), __receivedTriggers=%u"), __receivedTriggers);
} // fl::core::MotionDetectorTest::ConstAcceleration

} // fl::core

} // fl
