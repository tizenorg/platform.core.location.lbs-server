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
 * @file    MotionDetector.h
 * @brief   Implementation of an accelerometer-based motion detector
 */

#pragma once
#ifndef __FL_MOTION_DETECTOR_H__
#define __FL_MOTION_DETECTOR_H__

#include "core/Types.h"
#include "LPFilter.h"
#include "AccelerationReceiver.h"

namespace fl {
namespace core {

#define MDF_POSITION_SIGMA        real(2.5)  // typical GPS position measurement uncertainty, in [m]
#define MDF_VELOCITY_SIGMA        real(0.1)  // typical GPS velocity measurement uncertainty, in [m/s]
#define MDF_UNCERTAINTY_THRESHOLD real(50.0) // uncertainty threshold in BALANCED mode, in [m]
#define MDF_SAMPLING_FREQUENCY    real(10.0) // default sensor sampling frequency [Hz] (100ms interval)

class MotionDetector: AccelerationReceiver {
public:

	//! Interface for receiving notifications
	class Receiver {
	public:
		virtual void onSignificantMotionDetected(void) = 0;
	};

	//! Constructor
	MotionDetector(Receiver&);

	//! Implementation of fl::core::AccelerationReceiver::onAccelerationData
	void onAccelerationData(real t, AccelerationVector av) override;

protected:
	Receiver& __receiver;
	LPFilter  __lpAccelFilter;
	    real  __A0;
	    real  __A1;
	    real  __A2;
	    real  __triggerTime;   // last triggering time
	    real  __tau1;          // last accel receive time [s] (relative to last trigger)
	    real  __s2a1;          // last accel sigma^2(_tau1) [m^2/s^4]
	    real  __sigma2;        // estimated variance recorded for testing purposes

	//! Estimate the uncertainty in position via variance sigma2(t)
	bool checkVariance(real tau, real s2a);
	//! Reset inter-measurement integrals
	void reset(void);

}; // fl::core::MotionDetector

} // fl::core

} // fl

#endif // __FL_MOTION_DETECTOR_H__
