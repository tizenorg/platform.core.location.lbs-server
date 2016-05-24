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
 * @brief   Client specification
 */

#pragma once

#include "core/Types.h"
#include "ipc/Server.h"

#include "common/Logger.h"

#include <array>
#include <chrono>
#include <memory>
#include <string>

namespace fl {

class ClientNode final {
public:
	struct AccuracyBasedSettings {
		std::chrono::milliseconds desiredInterval = std::chrono::milliseconds::max();
		std::chrono::milliseconds minimalInterval = std::chrono::milliseconds::max();

		AccuracyBasedSettings() = default;
		AccuracyBasedSettings(const std::chrono::milliseconds& desiredInterval,
		                      const std::chrono::milliseconds& minimalInterval)
		    : desiredInterval(desiredInterval)
		    , minimalInterval(minimalInterval)
		{
		}
	};

	class Defaults {
	private:
		std::array<AccuracyBasedSettings, 3> settings;

	public:
		Defaults() = default;

		const AccuracyBasedSettings& operator[](core::AccuracyLevel level) const
		{
			return settings[static_cast<size_t>(level)];
		}

		AccuracyBasedSettings& operator[](core::AccuracyLevel level)
		{
			return settings[static_cast<size_t>(level)];
		}
	};

	ClientNode(const Defaults& defaults)
	    : defaults(defaults)
	{
		setAccuracyLevel(core::AccuracyLevel::NO_POWER);
	}

	ClientNode(ClientNode&&) = default;
	ClientNode(const ClientNode&) = default;
	ClientNode& operator=(ClientNode&&) = default;
	ClientNode& operator=(const ClientNode&) = default;

	const std::chrono::milliseconds& getDesiredInterval() const;
	const std::chrono::milliseconds& getMinimalInterval() const;
	core::AccuracyLevel getAccuracyLevel() const;
	void setDesiredInterval(const std::chrono::milliseconds& interval);
	void setMinimalInterval(const std::chrono::milliseconds& interval);
	void setAccuracyLevel(core::AccuracyLevel level);
	void setProxy(ipc::Server::ClientProxy* proxy);
	void sendUpdate(const core::Position& position) const;

private:
	std::chrono::milliseconds desiredInterval = std::chrono::milliseconds::max();
	std::chrono::milliseconds minimalInterval = std::chrono::milliseconds::max();
	core::AccuracyLevel accuracyLevel = core::AccuracyLevel::NO_POWER;
	std::shared_ptr<ipc::Server::ClientProxy> proxy = nullptr;
	const Defaults& defaults;

	bool overrideDefaults = false;
};
}
