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
 * @brief   Server for handling clients communication and settings
 */

#include "ClientNode.h"

namespace fl {

const std::chrono::milliseconds& ClientNode::getDesiredInterval() const { return desiredInterval; }

const std::chrono::milliseconds& ClientNode::getMinimalInterval() const { return minimalInterval; }

core::AccuracyLevel ClientNode::getAccuracyLevel() const { return accuracyLevel; }

void ClientNode::setDesiredInterval(const std::chrono::milliseconds& interval)
{
	desiredInterval = interval;
	overrideDefaults = true;
}

void ClientNode::setMinimalInterval(const std::chrono::milliseconds& interval)
{
	minimalInterval = interval;
	overrideDefaults = true;
}

void ClientNode::setAccuracyLevel(core::AccuracyLevel level)
{
	accuracyLevel = level;

	if (!overrideDefaults) {
		const auto& settings = defaults[level];
		desiredInterval = settings.desiredInterval;
		minimalInterval = settings.minimalInterval;
	}
}

void ClientNode::setProxy(ipc::Server::ClientProxy* proxy) { this->proxy.reset(proxy); }

void ClientNode::sendUpdate(const core::Position& position) const
{
	if (proxy != nullptr) {
		proxy->sendPosition(position);
	}
}
}
