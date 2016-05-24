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
 * @author  Pawel Kubik (p.kubik@samsung.com)
 * @brief   Service main class
 */

#pragma once

#include "GPSController.h"
#include "AccelerometerController.h"
#include "Server.h"

#include "core/Engine.h"

#include "common/Result.h"

namespace fl {

class Service final : public core::PositionReceiver {
public:
	Service();
	Result start();

	virtual void updatePosition(const core::Position& position);

private:
	GPSController gpsController;
	AccelerometerController accelerometerController;
	core::Engine engine;
	Server server;
};
}
