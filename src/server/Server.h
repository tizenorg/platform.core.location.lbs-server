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

#pragma once

#include "ClientNode.h"

#include "core/Engine.h"
#include "core/PositionReceiver.h"
#include "ipc/Server.h"

#include <string>
#include <unordered_map>

namespace fl {

class Server final : public ipc::Server::Handler, public core::PositionReceiver {
public:
	typedef ipc::Server::ClientId ClientId;
	typedef ipc::Server::ClientProxy ClientProxy;

	Server(core::Engine& engine);

	virtual std::string echo(const std::string& message);
	virtual void onStart();
	virtual bool registerClient(const ClientId& clientId);
	virtual bool unregisterClient(const ClientId& clientId);
	virtual bool setAccuracyLevel(const ClientId& clientId, const ipc::AccuracyLevel& level);
	virtual bool setDesiredInterval(const ClientId& clientId, const unsigned interval);
	virtual bool subscribe(const ClientId& clientId,
	                       const unsigned interval,
	                       std::unique_ptr<ClientProxy>&& proxy);
	virtual void updatePosition(const core::Position& position);

private:
	ipc::Server server;
	core::Engine& engine;
	ClientNode::Defaults defaults;
	std::unordered_map<ipc::Server::ClientId, ClientNode> clients;

	core::AccuracyLevel maxAccuracyLevel = core::AccuracyLevel::NO_POWER;
	std::chrono::milliseconds minDesiredInterval = std::chrono::milliseconds::max();
	void updateSettings();
	void sendUpdates(const core::Position& position) const;
};
}
