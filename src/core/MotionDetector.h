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
#include "core/Earth.h"
#include "LPFilter.h"
#include "AccelerationReceiver.h"

namespace fl {
namespace core {

class MotionDetector: public AccelerationReceiver {
public:
	static constexpr real SAMPLING_FREQUENCY    = static_cast<real>(10.0); // default sensor sampling frequency [Hz] (100ms interval)
	static constexpr real NOISE_LEVEL           = static_cast<real>( 0.2); // accelerometer's noise level [m/s^2]
	static constexpr real MOTION_LEVEL          = static_cast<real>( 2.0); // acceleration level of walking [m/s^2]

	enum State {
		UNDECIDED,  // initial
		MOVEMENT,   // acceleration above MOTION_LEVEL
		IMMOBILITY, // acceleration below NOISE_LEVEL
		SLEEP,      // immobile for IMMOBILITY_INTERVAL
	};

	//! Interface for receiving notifications
	class Receiver {
	public:
		//! Invoked when the motion state changes
		virtual void onMotionState(State) = 0;
	};

	//! Constructor
	MotionDetector(Receiver&);

	//! Implementation of fl::core::AccelerationReceiver::onAccelerationData
	void onAccelerationData(real t, AccelerationVector av) override;

protected:
	static constexpr   real INTEGRATION_INTERVAL         = static_cast<real>(10.0); // [s]
	static constexpr   real IMMOBILITY_INTERVAL          = static_cast<real>(30.0); // [s]
	static constexpr   real IMMOBILITY_LEVEL             = static_cast<real>(square(NOISE_LEVEL)     / 18 * (NOISE_LEVEL     * M_PI + 4 * square(earth::GRAVITY)));
	static constexpr   real IMMOBILITY_THRESHOLD         = static_cast<real>(square(NOISE_LEVEL * 2) / 18 * (NOISE_LEVEL * 2 * M_PI + 4 * square(earth::GRAVITY)));
	static constexpr   real MOVEMENT_THRESHOLD           = static_cast<real>(square(MOTION_LEVEL   ) / 18 * (MOTION_LEVEL    * M_PI + 4 * square(earth::GRAVITY)));
	static constexpr double CALIBRATION_INTERVAL         = 24*60*60;   // [s]
	static constexpr double CALIBRATION_REPLACEMENT_RATE = (1.0 / (SAMPLING_FREQUENCY * CALIBRATION_INTERVAL));
	static constexpr double CALIBRATION_DECAY_RATE       = (1.0 - CALIBRATION_REPLACEMENT_RATE);

	   State  __state;
	   State  __lastNotification;
	Receiver& __receiver;
	LPFilter  __lpAccelFilter;
	  double  __g2;           // long term exponential moving average of <a^2> [m^2/s^4]
	    real  __s22a;         // exponential moving average of <[(a^2 - g^2)/3]^2> [m^4/s^8]
	    real  __rmaDecayRate;
	    real  __rmaReplacementRate;
	    real  __immobileTime; // last triggering time

	void setSamplingFrequency(real samplingFrequency);
	void setState(State state);

}; // fl::core::MotionDetector

} // fl::core

} // fl

#endif // __FL_MOTION_DETECTOR_H__
