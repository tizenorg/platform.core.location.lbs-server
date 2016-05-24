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

#include "Server.h"

#include "common/Logger.h"

namespace fl {

namespace {

std::string levelToString(core::AccuracyLevel level)
{
	static const std::string names[] = {"NO_POWER", "BALANCED_POWER", "HIGH_ACCURACY"};
	return names[size_t(level)];
}

}

Server::Server(core::Engine& engine)
    : server(*this)
    , engine(engine)
{
	constexpr auto SECOND = std::chrono::seconds(1);
	constexpr auto MINUTE = 60 * SECOND;
	constexpr auto HOUR = 60 * MINUTE;
	constexpr auto DAY = 24 * HOUR;

	// TODO: Read settings at runtime
	// Note that only the first parameter is currently meaningful
	defaults[core::AccuracyLevel::NO_POWER] = ClientNode::AccuracyBasedSettings{DAY, HOUR};
	defaults[core::AccuracyLevel::BALANCED_POWER] =
	        ClientNode::AccuracyBasedSettings{20 * MINUTE, 2 * MINUTE};
	defaults[core::AccuracyLevel::HIGH_ACCURACY] =
	        ClientNode::AccuracyBasedSettings{SECOND, SECOND};
	clients.emplace(std::make_pair("DefaultSettings", ClientNode{defaults}));
}

std::string Server::echo(const std::string& message) { return message; }

void Server::onStart()
{
	LOGI("Server started.");

	//    ::g_timeout_add(1000, [](gpointer user_data) -> gboolean
	//    {
	//        auto server = static_cast<Server*>(user_data);

	//        core::Position position{111, 1.0, 2.0, 3.14};
	//        server->engine.updatePosition(core::SourceType::GPS, position);

	//        return TRUE;
	//    }, this);
}

bool Server::registerClient(const ipc::Server::ClientId& clientId)
{
	const auto& ret = clients.emplace(clientId, ClientNode(defaults));
	if (ret.second == false) {
		LOGW("Client (" + clientId + ") is already registered");
		return false;
	} else {
		LOGI("Registered client: (" + clientId + ")");
		return true;
	}
}

void Server::updateSettings()
{
	auto oldAccuracyLevel = maxAccuracyLevel;
	auto oldDesiredInterval = minDesiredInterval;
	maxAccuracyLevel = core::AccuracyLevel::NO_POWER;
	minDesiredInterval = std::chrono::milliseconds::max();

	for (const auto& c : clients) {
		maxAccuracyLevel = std::max(maxAccuracyLevel, c.second.getAccuracyLevel());
		minDesiredInterval = std::min(minDesiredInterval, c.second.getDesiredInterval());
	}

	if (maxAccuracyLevel != oldAccuracyLevel) {
		engine.setAccuracy(maxAccuracyLevel);
	}

	if (minDesiredInterval != oldDesiredInterval) {
		engine.setInterval(minDesiredInterval);
	}
}

void Server::sendUpdates(const core::Position& position) const
{
	for (auto it : clients) {
		auto& client = it.second;
		client.sendUpdate(position);
	}
}

bool Server::unregisterClient(const ipc::Server::ClientId& clientId)
{
	const auto& ret = clients.find(clientId);
	if (ret != clients.end()) {
		ClientNode client = ret->second;
		clients.erase(ret);

		bool mightAffectGlobal = client.getAccuracyLevel() == maxAccuracyLevel ||
		                         client.getDesiredInterval() == minDesiredInterval;
		if (mightAffectGlobal) {
			updateSettings();
		}

		LOGI("Unregistered client: (" + clientId + ")");
		LOGI("New general settings: " + levelToString(maxAccuracyLevel) + ", desired interval: " +
		     std::to_string(minDesiredInterval.count()) + "ms");
		return true;
	} else {
		LOGW("Failed to unregister non-registered client (" + clientId + ")");
		return false;
	}
}

bool Server::setAccuracyLevel(const Server::ClientId& clientId, const ipc::AccuracyLevel& level)
{
	const auto& ret = clients.find(clientId);
	if (ret != clients.end()) {
		auto& client = ret->second;
		bool mightAffectGlobal = client.getAccuracyLevel() == maxAccuracyLevel;
		client.setAccuracyLevel(level);

		if (level > maxAccuracyLevel) {
			maxAccuracyLevel = level;
			engine.setAccuracy(level);
		} else if (mightAffectGlobal) {
			updateSettings();
		}

		LOGI("The accuracy of (" + clientId + ") was set to " + levelToString(level));

		return true;
	} else {
		LOGE("Failed to set property. Use register the client first!");
		return false;
	}
}

bool Server::setDesiredInterval(const Server::ClientId& clientId, const unsigned interval)
{
	const auto chronoInterval = std::chrono::milliseconds(interval);

	const auto& ret = clients.find(clientId);
	if (ret != clients.end()) {
		auto& client = ret->second;
		bool mightAffectGlobal = client.getDesiredInterval() == minDesiredInterval;
		client.setDesiredInterval(chronoInterval);

		if (chronoInterval < minDesiredInterval) {
			minDesiredInterval = chronoInterval;
			engine.setInterval(chronoInterval);
		} else if (mightAffectGlobal) {
			updateSettings();
		}

		LOGI("The desired interval of (" + clientId + ") was set to " + std::to_string(interval) +
		     "ms");

		return true;
	} else {
		LOGE("Failed to set property. Use register the client first!");
		return false;
	}
}

bool Server::subscribe(const Server::ClientId& clientId,
                       const unsigned interval,
                       std::unique_ptr<Server::ClientProxy>&& proxy)
{
	const auto chronoInterval = std::chrono::milliseconds(interval);

	const auto& ret = clients.find(clientId);
	if (ret != clients.end()) {
		auto& client = ret->second;
		client.setMinimalInterval(chronoInterval);
		client.setProxy(proxy.release());

		return true;
	} else {
		LOGE("Failed to subscribe the client. Register the client first!");
		return false;
	}
}

void Server::updatePosition(const core::Position& position)
{
	sendUpdates(position);
}

}
