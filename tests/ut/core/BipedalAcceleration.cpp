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

#include "core/Earth.h"
#include "BipedalAcceleration.h"

using namespace fl::core;

const BipedalAcceleration::ModeConstants BipedalAcceleration::__modeConstants[BipedalAcceleration::Mode::COUNT] = {
//  {amplitude, distortion}
	{2.0,       1.5}, // Mode::WALK
	{5.0,       2.0}, // Mode::JOGTROT
	{9.0,       2.5}  // Mode::JUMP
};

BipedalAcceleration::BipedalAcceleration(Mode mode)
{
	__c = __modeConstants[mode].distortion;
	__b = __modeConstants[mode].amplitude /sinh(__c);
	__a = earth::GRAVITY + __b * besselI0(__c);
}

real fl::core::BipedalAcceleration::get(double phi)
{
	return static_cast<real>(__a - __b * exp(__c * sin(phi)));
}
