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

#if !defined(LOG_THRESHOLD)
	// custom logging threshold - keep undefined on the repo
	// #define LOG_THRESHOLD   LOG_LEVEL_INFO
#endif

#include "Engine.h"
#include "common/Logger.h"

namespace fl {
namespace core {

void Engine::setAccuracy(const AccuracyLevel& accuracy) { this->__accuracy = accuracy; }

void Engine::setInterval(const std::chrono::milliseconds& interval)
{
	this->__interval = interval;
	__intervalReceiver.updateInterval(interval);
}

void Engine::updatePosition(SourceType, const Position& position)
{
	// forward every update
	__positionReceiver.updatePosition(position);
}

void Engine::onMotionState(MotionDetector::State state)
{
	LOG_TRACE(FUNCTION_THIS_PREFIX("(state=%u)"), state);

	switch (state) {
	case MotionDetector::State::MOVEMENT:
		__onMotionDetected();
		break;
	case MotionDetector::State::SLEEP:
		__onIdleDetected();
		break;
	default:
		break;
	}
}

void Engine::__onMotionDetected()
{
	if (__idle) {
		LOG_INFO("Switching to active state...");
		__idle = false;
		__intervalReceiver.updateInterval(__interval);
	}
}

void Engine::__onIdleDetected()
{
	if (!__idle) {
		LOG_INFO("Switching to the idle state...");
		__idle = true;
		__intervalReceiver.updateInterval(std::chrono::milliseconds::max());
	}
}

}
}
