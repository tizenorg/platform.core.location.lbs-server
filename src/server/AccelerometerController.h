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
 * @file
 * @brief   Accelerometer controller class
 */

#pragma once

#include <cstdint>
#include <sensor.h>
#include "core/AccelerationReceiver.h"
#include "common/Result.h"

namespace fl {

class AccelerometerController final {
public:
	Result init();
	Result start(core::AccelerationReceiver& receiver);
	Result stop();
	Result shutdown();

	~AccelerometerController();

private:
	sensor_h __sensor = nullptr;
	sensor_listener_h __listener = nullptr;

	Result __setAlwaysOn();
};
}
