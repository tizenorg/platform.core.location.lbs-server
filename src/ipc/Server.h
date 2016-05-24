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
 * @brief   Server IPC interface
 */

#pragma once

#include "Common.h"

#include <gio/gio.h>

#include <memory>
#include <string>

namespace fl {
namespace ipc {

class Server final {
public:
	/** Client identifier.
	 *
	 *  Hashable, Comparable, Streamable, Copyable
	 */
	typedef std::string ClientId;

	class ClientProxy;
	class Handler;

	Server(Handler& handler);
	~Server();

	void onBusAcquired(GDBusConnection* connection);
	void onNameAcquired();
	void onNameLost();
	void onMethodCall(const gchar* sender,
	                  const gchar* /*object_path*/,
	                  const gchar* /*interface_name*/,
	                  const gchar* methodName,
	                  GVariant* parameters,
	                  GDBusMethodInvocation* invocation);
	void onAnyNameVanished(const std::string& name);

private:
	typedef std::unique_ptr<GDBusNodeInfo, void (*)(GDBusNodeInfo*)> IntrospectionPtr;

	Handler& handler;
	GDBusConnection* connection = nullptr;
	IntrospectionPtr introspection = IntrospectionPtr(nullptr, ::g_dbus_node_info_unref);
	guint nameOwnerId = 0;
	guint registrationId = 0;

	void registerClient(const ClientId& clientId);
};

class Server::ClientProxy {
private:
	int fd[2];

public:
	ClientProxy();
	int getInput() const;
	bool sendPosition(const Position& position);
	~ClientProxy();
};

class Server::Handler {
public:
	virtual std::string echo(const std::string& message) = 0;
	virtual void onStart() = 0;
	virtual bool registerClient(const ClientId& clientId) = 0;
	virtual bool unregisterClient(const ClientId& clientId) = 0;
	virtual bool setAccuracyLevel(const ClientId& clientId, const AccuracyLevel& level) = 0;
	virtual bool setDesiredInterval(const ClientId& clientId, const unsigned interval) = 0;
	virtual bool subscribe(const ClientId& clientId,
	                       const unsigned interval,
	                       std::unique_ptr<ClientProxy>&& proxy) = 0;
};
}
}
