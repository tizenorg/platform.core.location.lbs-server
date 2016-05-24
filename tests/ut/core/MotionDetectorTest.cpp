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
#include "core/Types.h"
#include "core/Earth.h"
#include "common/MathExt.h"
#include "common/Logger.h"
#include "BipedalAcceleration.h"

namespace fl {
namespace core {

static constexpr    real LOG_PRECISION     = static_cast<real>   (0.1);   // maximal log(a/b)^2
static constexpr Seconds TEST_START_TIME   = static_cast<Seconds>(3.14);  // [s] anything greater than zero
static constexpr Seconds TEST_INTERVAL     = static_cast<Seconds>(100.0); // [s]
static constexpr Seconds SAMPLING_INTERVAL = static_cast<Seconds>(1.0 / MotionDetector::SAMPLING_FREQUENCY);
static constexpr   Hertz WALKING_FREQUENCY = static_cast<Hertz>  (2.0);   // [Hz]

void MotionDetectorTest::onMotionState(State state)
{
	switch (state) {
	case MotionDetector::State::MOVEMENT:
		LOG_TRACE(FUNCTION_THIS_PREFIX("(), __receivedMovementTriggers:%u->%u"), __receivedMovementTriggers, __receivedMovementTriggers + 1);
		__receivedMovementTriggers++;
		break;

    case MotionDetector::State::SLEEP:
		LOG_TRACE(FUNCTION_THIS_PREFIX("(), __receivedImmobilityTriggers:%u->%u"), __receivedImmobilityTriggers, __receivedImmobilityTriggers + 1);
		__receivedImmobilityTriggers++;
		break;

    default:
    //// MotionDetector::State::UNDECIDED:
    //// MotionDetector::State::IMMOBILITY:
        break;
	}
} // fl::core::MotionDetectorTest::onMotionState

/*******************************************************************//**
 *	Simplest test with no measurement uncertainty, and no acceleration:
 *	expecting sigma2 = 0 all the time.
 */
TEST_F(MotionDetectorTest, Idle)
{
	AccelerationVector av;

	LOG_TRACE(ENTER_FUNCTION_THIS_PREFIX("()"));
	av[SPATIAL_X] = 0;
	av[SPATIAL_Y] = 0;
	av[SPATIAL_Z] = earth::GRAVITY;
#ifndef NDEBUG
	__calibration.status  = COMPLETE;
	__calibration.samples = CALIBRATION_SAMPLES;
	__g2 = square(earth::GRAVITY);
#endif
	// we do not expect any events
	__receivedMovementTriggers = 0;
	for (Seconds dt = 0;  dt < TEST_INTERVAL;  dt += SAMPLING_INTERVAL)
		onAccelerationData(TEST_START_TIME + dt, av);
	EXPECT_EQ(__receivedMovementTriggers, 0);
	LOG_TRACE(LEAVE_FUNCTION_THIS_PREFIX("()"));
} // fl::core::MotionDetectorTest::Idle

/*******************************************************************//**
 *	Assume uncalibrated accelerometer with some deviation from the true
 *	Earth's gravity. Test the adaptation mechanism.
 */
TEST_F(MotionDetectorTest, GravityCalibration)
{
	constexpr     real MIN_G =-10.5;
	constexpr     real MAX_G = -9.5;
	AccelerationVector av;

	LOG_TRACE(ENTER_FUNCTION_THIS_PREFIX("()"));
	for (real g = MIN_G;  g <= MAX_G;  g += static_cast<real>(1.0 / 16)) {
	#ifndef NDEBUG
		__calibration.status  = UNINITIALIZED;
		__calibration.samples = 0;
	#endif
		for (Seconds dt = 0; dt < 4 * CALIBRATION_INTERVAL; dt += SAMPLING_INTERVAL) {
			av[SPATIAL_X] = rand() * (NOISE_LEVEL / RAND_MAX) - NOISE_LEVEL / 2;
			av[SPATIAL_Y] = rand() * (NOISE_LEVEL / RAND_MAX) - NOISE_LEVEL / 2;
			av[SPATIAL_Z] = rand() * (NOISE_LEVEL / RAND_MAX) - NOISE_LEVEL / 2 + g;
			onAccelerationData(TEST_START_TIME + dt, av);
		}
		// we do not expect any events
		EXPECT_EQ(__receivedMovementTriggers, 0);
		EXPECT_EQ(__receivedImmobilityTriggers, 1);
		__receivedMovementTriggers   = 0;
		__receivedImmobilityTriggers = 0;
		setState(UNDECIDED);
		double estimated_g = -sqrt(__g2);
		EXPECT_LE(square(estimated_g - g), 0.01);
	}
	LOG_TRACE(LEAVE_FUNCTION_THIS_PREFIX("()"));
} // fl::core::MotionDetectorTest::Idle

/*******************************************************************//**
 *	Switch between oscillatory (waliking-like), and idle states 10
 *	times and ensure the motion triggers.
 */
TEST_F(MotionDetectorTest, Itinerant)
{
	 constexpr unsigned CYCLES = 10;
	 AccelerationVector av;
	             double t;
	BipedalAcceleration walkingAcceleration(BipedalAcceleration::WALK);

	LOG_TRACE(ENTER_FUNCTION_THIS_PREFIX("()"));
	av[SPATIAL_X] = 0;
	av[SPATIAL_Y] = 0;
	av[SPATIAL_Z] = earth::GRAVITY;
#ifndef NDEBUG
	__calibration.status  = COMPLETE;
	__calibration.samples = CALIBRATION_SAMPLES;
	__g2 = square(earth::GRAVITY);
#endif
	t = TEST_START_TIME;
	__s22a = square(NOISE_LEVEL);
	for (unsigned n = 0;  n < CYCLES;  ++n) {
		// simulate walk
		for (Seconds dt = 0; dt < TEST_INTERVAL; dt += SAMPLING_INTERVAL) {
			av[SPATIAL_X] = rand() * (NOISE_LEVEL / RAND_MAX) - NOISE_LEVEL / 2;
			av[SPATIAL_Y] = rand() * (NOISE_LEVEL / RAND_MAX) - NOISE_LEVEL / 2;
			av[SPATIAL_Z] = walkingAcceleration.get(dt * WALKING_FREQUENCY);
			onAccelerationData(t, av);
			t += SAMPLING_INTERVAL;
			if (__receivedMovementTriggers > 0)
				break;
		}
		if (__receivedMovementTriggers == 0) {
			LOG_ERROR("n=%u, t=%g, __receivedMovementTriggers=0", n, t);
			EXPECT_NE(__receivedMovementTriggers, 0);
		}
		else
			LOG_TRACE("n=%u, t=%g, __receivedMovementTriggers=%u", n, t, __receivedMovementTriggers);
		EXPECT_EQ(__state, MotionDetector::MOVEMENT);
		// clean-up
		__receivedMovementTriggers   = 0;
		__receivedImmobilityTriggers = 0;
		// simulate rest
		for (Seconds dt = 0; dt < IMMOBILITY_INTERVAL + 4 * INTEGRATION_INTERVAL; dt += SAMPLING_INTERVAL) {
			av[SPATIAL_X] = rand() * (NOISE_LEVEL / RAND_MAX) - NOISE_LEVEL / 2;
			av[SPATIAL_Y] = rand() * (NOISE_LEVEL / RAND_MAX) - NOISE_LEVEL / 2;
			av[SPATIAL_Z] = rand() * (NOISE_LEVEL / RAND_MAX) - NOISE_LEVEL / 2 + earth::GRAVITY;
			onAccelerationData(t, av);
			t += SAMPLING_INTERVAL;
			if (__receivedImmobilityTriggers > 0)
				break;
		}
		if (__receivedImmobilityTriggers == 0) {
			LOG_ERROR("n=%u, t=%g, __receivedImmobilityTriggers=0", n, t);
			EXPECT_NE(__receivedImmobilityTriggers, 0);
		}
		else
			LOG_TRACE("n=%u, t=%g, __receivedImmobilityTriggers=%u", n, t, __receivedImmobilityTriggers);
		EXPECT_EQ(__state, MotionDetector::SLEEP);
		// clean-up
		__receivedMovementTriggers   = 0;
		__receivedImmobilityTriggers = 0;
	}
	LOG_TRACE(LEAVE_FUNCTION_THIS_PREFIX("()"));
}
} // fl::core

} // fl
