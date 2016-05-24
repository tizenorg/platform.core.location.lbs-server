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
 * @file    core/AccelerationReceiver.h
 * @brief   Accelerometer sensor data interface.
 */

#pragma once
#ifndef __FL_ACCELARATION_RECEIVER_H__
#define __FL_ACCELARATION_RECEIVER_H__

#include "common/MathExt.h"

namespace fl {

namespace core {

enum Spatial {
	SPATIAL_X = 0,
	SPATIAL_Y = 1,
	SPATIAL_Z = 2,
	SPATIAL_DIMENSION
};

typedef real AccelerationVector[SPATIAL_DIMENSION];

class AccelerationReceiver {
public:
	/** Accelerometer data input
	 *
	 * @param[in] t
	 *    Event time in [s]
	 * @param[in] a[]
	 *    Array containing 3 spatial acceleration components.
	 */
	virtual void onAccelerationData(real t, AccelerationVector av) = 0;
}; // fl::AccelerationReceiver

} // core

} // fl

#endif // __FL_ACCELARATION_RECEIVER_H__
