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

#define MDF_POSITION_SIGMA2         SQUARE(MDF_POSITION_SIGMA)
#define MDF_VELOCITY_SIGMA2         SQUARE(MDF_VELOCITY_SIGMA)
#define MDF_UNCERTAINTY_THRESHOLD2  SQUARE(MDF_UNCERTAINTY_THRESHOLD)

fl::core::MotionDetector::MotionDetector(Receiver& receiver)
: __receiver(receiver)
, __triggerTime(0)
{
	LOG_TRACE(FUNCTION_THIS_PREFIX("(%p)"), &receiver);
	__lpAccelFilter.reset(MDF_SAMPLING_FREQUENCY);
	reset();
	__s2a1 = 0;
} // fl::core::MotionDetector::MotionDetector

void fl::core::MotionDetector::reset()
{
	LOG_TRACE(FUNCTION_THIS_PREFIX("()"));
	__A0     = 0;
	__A1     = 0;
	__A2     = 0;
	__tau1   = 0;
	__sigma2 = 0;
} // fl::core::MotionDetector::reset

void fl::core::MotionDetector::onAccelerationData(real t, AccelerationVector a)
{
	real dt;
	real s2a;

	dt = (__triggerTime > 0 ? t - __triggerTime : 0);
	LOG_TRACE(FUNCTION_THIS_PREFIX("(t=%g, a=(%g, %g, %g)), dt=%g"), t, a[SPATIAL_X], a[SPATIAL_Y], a[SPATIAL_Z], dt);
	// <s^2> = <(a^2 - g^2)/3>
	s2a = __lpAccelFilter.process((square(a[SPATIAL_X]) + square(a[SPATIAL_Y]) + square(a[SPATIAL_Z]) - SQUARE(LPF_GRAVITY)) / 3);
	if (dt > 0) {
		// estimate uncertainties
		if (checkVariance(dt, s2a)) {
			__receiver.onSignificantMotionDetected();
			__triggerTime = t;
			reset();
		}
	} else
		__triggerTime = t;
} // fl::core::MotionDetector::onAccelData

bool fl::core::MotionDetector::checkVariance(real tau, real s2a)
{
	real Qpp; // Q(x,x) = Q(y,y)

	LOG_TRACE(FUNCTION_THIS_PREFIX("(tau=%g, s2a=%g)"), tau, s2a);
	// integral(tau1:tau0, s2)
	__A0 += (tau - __tau1) * (s2a + __s2a1) / 2;
	// integral(tau1:tau0, t*s2)
	__A1 += (tau + __tau1) * (s2a * tau - __s2a1 * __tau1) / 6 + (s2a * square(tau) - __s2a1 * square(__tau1)) / 3;
	// integral(tau1:tau0, t^2*s2)
	__A2 += (square(tau) + tau * __tau1 + square(__tau1)) * (__s2a1 * tau - s2a * __tau1)/12 + (s2a * cube(tau) - __s2a1 * cube(__tau1)) / 4;
	// independent elements of the Q-matrix:
	Qpp = tau*(tau * __A0 - 2 * __A1) + __A2;
	// Sp = Q + F.Sm.F^T
	__sigma2 = MDF_POSITION_SIGMA2 + square(tau)*MDF_VELOCITY_SIGMA2 + Qpp;
	// remember last values
	__tau1 = tau;
	__s2a1 = s2a;
	// detector
	return (__sigma2 >= MDF_UNCERTAINTY_THRESHOLD2);
} // fl::core::MotionDetector::_estimateS
