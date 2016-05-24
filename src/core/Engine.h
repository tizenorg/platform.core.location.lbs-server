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
 * @brief   Fused Location engine
 */

#pragma once

#include "IntervalReceiver.h"
#include "PositionReceiver.h"
#include "MotionDetector.h"
#include "Types.h"

#include <chrono>

namespace fl {
namespace core {

class Engine final : public MotionDetector::Receiver {
public:
	Engine(PositionReceiver& positionReceiver,
	       IntervalReceiver& intervalReceiver)
	    : __positionReceiver(positionReceiver)
	    , __intervalReceiver(intervalReceiver)
	    , __motionDetector(*this)
	{
	}

	void setAccuracy(const AccuracyLevel& __accuracy);
	void setInterval(const std::chrono::milliseconds& __interval);
	void updatePosition(SourceType, const Position& position);
	AccelerationReceiver& getAccelerationReceiver() { return __motionDetector; }

	void onMotionState(MotionDetector::State state) override;

private:
	PositionReceiver& __positionReceiver;
	IntervalReceiver& __intervalReceiver; // split this receiver to handle more location methods
	MotionDetector __motionDetector;
	AccuracyLevel __accuracy = AccuracyLevel::NO_POWER;
	std::chrono::milliseconds __interval = std::chrono::milliseconds::max();
	bool __idle = false;

	void __onMotionDetected();
	void __onIdleDetected();
};
}
}
