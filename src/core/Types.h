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
 * @file    core/Types.h
 * @brief   Types definitions
 */

#pragma once

#include <cstdint>

namespace fl {
namespace core {

//! Time variables in SI units
typedef double Seconds;
//! Frequency variables in SI units
typedef float Hertz;

enum class AccuracyLevel : uint8_t { NO_POWER = 0, BALANCED_POWER, HIGH_ACCURACY };

struct Position {
	int timestamp;
	double latitude;
	double longitude;
	double altitude;
};

enum class SourceType : uint8_t { GPS, WPS };
}
}
