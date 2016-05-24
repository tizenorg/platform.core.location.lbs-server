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
 * @brief   GPS controller class
 */

#pragma once

#include "core/IntervalReceiver.h"
#include "core/PositionReceiver.h"

#include "lbs_dbus_client.h"

namespace fl {

class GPSController final : public core::IntervalReceiver {
public:
	GPSController(core::PositionReceiver& receiver)
	    : __receiver(receiver)
	{
	}

	void start();
	void updateInterval(const std::chrono::milliseconds& interval);

	~GPSController();

private:
	static constexpr unsigned INITIAL_INTERVAL = 10000; //ms
	core::PositionReceiver& __receiver;
	lbs_client_dbus_h __lbsHandle = nullptr;

	void __onPositionChanged(GVariant* parameters);
	void __onStatusChanged(GVariant* parameters);
	void __updatePosition(const core::Position& position);
};
}
