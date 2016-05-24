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

/**
 * @file    MotionDetector.cpp
 * @brief   Implementation of an accelerometer-based motion detector
 */

#if !defined(LOG_THRESHOLD)
	// custom logging threshold - keep undefined on the repo
	// #define LOG_THRESHOLD   LOG_LEVEL_TRACE
#endif

#include "MotionDetector.h"
#include "common/MathExt.h"
#include "common/Logger.h"


fl::core::MotionDetector::MotionDetector(Receiver& receiver)
: __state(UNDECIDED)
, __lastNotification(UNDECIDED)
, __receiver(receiver)
, __g2(square(earth::GRAVITY))
, __s22a(IMMOBILITY_LEVEL)
, __immobileTime(0)
{
	LOG_TRACE(FUNCTION_THIS_PREFIX("(%p)"), &receiver);
	setSamplingFrequency(SAMPLING_FREQUENCY);
	;
} // fl::core::MotionDetector::MotionDetector

void fl::core::MotionDetector::setSamplingFrequency(real samplingFrequency)
{
	__lpAccelFilter.reset(samplingFrequency);
	__rmaReplacementRate = static_cast<real>(1.0 / (samplingFrequency * INTEGRATION_INTERVAL)); // ~ 1/samples-to-average
	__rmaDecayRate       = 1 - __rmaReplacementRate;
} // fl::core::MotionDetector::setSamplingFrequency

void fl::core::MotionDetector::setState(State state) {
	LOG_TRACE(FUNCTION_THIS_PREFIX("(%u->%u)"), __state, state);
	__state = state;
	if (__lastNotification != state) {
		__receiver.onMotionState(state);
		__lastNotification = state;
	}
} // fl::core::MotionDetector::setState

void fl::core::MotionDetector::onAccelerationData(real t, AccelerationVector a)
{
	real s2a;
	real a2;

	LOG_TRACE(FUNCTION_THIS_PREFIX("(t=%g, a=(%g, %g, %g))"), t, a[SPATIAL_X], a[SPATIAL_Y], a[SPATIAL_Z]);
	a2 = square(a[SPATIAL_X]) + square(a[SPATIAL_Y]) + square(a[SPATIAL_Z]);
	// long-term average to correct for device miscalibration
	__g2 = a2 * CALIBRATION_REPLACEMENT_RATE + __g2 * CALIBRATION_DECAY_RATE;
	// <s^2> = <(a^2 - g^2)/3>
	s2a = __lpAccelFilter.process((a2 - static_cast<real>(__g2)) / 3);
	// update moving average: for a sinusoidal oscillation of amplitude A
	// around the Earth's gravity g the squared average is approximately
	// <s^4> ~ A^2/18(A*pi - 4g^2)
	__s22a = square(s2a) * __rmaReplacementRate +  __s22a * __rmaDecayRate;
	// estimate uncertainties
	if (__s22a > MOVEMENT_THRESHOLD) {
		switch(__state) {
		case UNDECIDED:
		case IMMOBILITY:
		case SLEEP:
			setState(MOVEMENT);
			break;

		default: // MOVEMENT
			break;
		}
	} else if (__s22a < IMMOBILITY_THRESHOLD) {
		switch (__state) {
		case UNDECIDED:
		case MOVEMENT:
			setState(IMMOBILITY);
			__immobileTime = t;
			break;

		case IMMOBILITY:
			if (t - __immobileTime > IMMOBILITY_INTERVAL)
				setState(SLEEP);
			break;

		default: // SLEEP
			break;
		}
	}
} // fl::core::MotionDetector::onAccelData
