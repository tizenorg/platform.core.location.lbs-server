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

#pragma once
#ifndef __FL_BIPEDAL_ACCELERATION_H__
#define __FL_BIPEDAL_ACCELERATION_H__

#include "common/MathExt.h"

namespace fl {
namespace core {

class BipedalAcceleration {
public:
	enum Mode {
		WALK,
		JOGTROT,
		JUMP,
		// always the last
		COUNT
	};

	BipedalAcceleration(Mode mode = WALK);

	real get(double phi);

private:
	struct ModeConstants {
		double amplitude;
		double distortion;
	};
	static const ModeConstants __modeConstants[Mode::COUNT];
	                    double __a;
	                    double __b;
	                    double __c;

}; // fl::core::BipedalAcceleration

} // fl::core

} // fl

#endif // __FL_BIPEDAL_ACCELERATION_H__
