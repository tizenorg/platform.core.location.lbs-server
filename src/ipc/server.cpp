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

#include "common.hpp"
#include "exception.hpp"
#include "server.hpp"
#include "dbus-error.hpp"

#include "common/logger.hpp"
#include "common/utils.hpp"

#include <gio/gunixfdlist.h>

namespace fl
{
namespace ipc
{

Server::Server(Handler& handler)
    : handler(handler)
{
    auto busAcquiredCb = [] (GDBusConnection* connection, const gchar*, gpointer obj) -> void
    {
        static_cast<Server*>(obj)->onBusAcquired(connection);
    };

    auto nameAcquiredCb = [] (GDBusConnection*, const gchar*, gpointer obj) -> void
    {
        static_cast<Server*>(obj)->onNameAcquired();
    };

    auto nameLostCb = [] (GDBusConnection*, const gchar*, gpointer obj) -> void
    {
        static_cast<Server*>(obj)->onNameLost();
    };

    nameOwnerId = ::g_bus_own_name(DBUS_BUS_TYPE,
                                   DBUS_SERVER_SERVICE_NAME,
                                   G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                   busAcquiredCb,
                                   nameAcquiredCb,
                                   nameLostCb,
                                   this,
                                   nullptr);
}

Server::~Server()
{
    if (nameOwnerId != 0)
    {
        ::g_bus_unown_name(nameOwnerId);
    }

    if (connection != nullptr)
    {
        if (registrationId != 0)
        {
            ::g_dbus_connection_unregister_object(connection, registrationId);
        }

        g_object_unref(connection);
    }
}

namespace
{

static const GDBusInterfaceVTable interfaceVTable =
{
    [] (GDBusConnection*       /*connection*/,
        const gchar*           sender,
        const gchar*           object_path,
        const gchar*           interface_name,
        const gchar*           method_name,
        GVariant*              parameters,
        GDBusMethodInvocation* invocation,
        gpointer               user_data)
    {
        static_cast<Server*>(user_data)->onMethodCall(sender, object_path, interface_name,
                                                      method_name, parameters, invocation);
    },
    nullptr,
    nullptr,
    0
};

}

void Server::onBusAcquired(GDBusConnection* connection)
{
    LOGI("DBus bus acquired.");
    this->connection = connection;
    Error error;

    introspection.reset(::g_dbus_node_info_new_for_xml(DBUS_INTERFACE_XML, error.pass()));

    if (introspection == nullptr)
    {
        const auto& msg = "Failed to parse DBus interface: " + error.getMessage();
        LOGE(msg);
        throw IPCException(msg);
    }

    registrationId = ::g_dbus_connection_register_object(connection,
                                                         DBUS_PATH_NAME,
                                                         // Server interface specified as 1st
                                                         introspection->interfaces[0],
                                                         &interfaceVTable,
                                                         this,
                                                         nullptr,
                                                         error.pass());

    if (registrationId == 0)
    {
        const auto& msg = "Failed to register DBus object: " + error.getMessage();
        LOGE(msg);
        throw IPCException(msg);
    }
}

void Server::onNameAcquired()
{
    LOGI("DBus service name acquired.");
    handler.onStart();
}

void Server::onNameLost()
{
    LOGE("DBus service name lost.");
}

void Server::onMethodCall(const gchar*           sender,
                          const gchar*           /*object_path*/,
                          const gchar*           /*interface_name*/,
                          const gchar*           methodName,
                          GVariant*              parameters,
                          GDBusMethodInvocation* invocation)
{
    // TODO: Consider using string->enum hashmap
    const std::string method(methodName);

    LOGI(std::string(sender) + " called " + methodName);

    if (method == "echo")
    {
        const gchar* message;
        ::g_variant_get(parameters, "(s)", &message);
        const std::string response = handler.echo(message);

        ::g_dbus_method_invocation_return_value(invocation,
                                                g_variant_new("(s)", response.c_str()));
    }
    else if (method == "registerClient")
    {
        registerClient(sender);

        ::g_dbus_method_invocation_return_value(invocation,
                                                g_variant_new("()"));
    }
    else if (method == "setAccuracy")
    {
        unsigned level;
        ::g_variant_get(parameters, "(u)", &level);

        handler.setAccuracyLevel(sender, AccuracyLevel(level));

        ::g_dbus_method_invocation_return_value(invocation,
                                                g_variant_new("()"));
    }
    else if (method == "setDesiredInterval")
    {
        unsigned interval;
        ::g_variant_get(parameters, "(u)", &interval);

        handler.setDesiredInterval(sender, interval);

        ::g_dbus_method_invocation_return_value(invocation,
                                                g_variant_new("()"));
    }
    else if (method == "subscribe")
    {
        unsigned interval;
        ::g_variant_get(parameters, "(u)", &interval);

        try {
            std::unique_ptr<ClientProxy> proxy(new ClientProxy());
            int inputFd = proxy->getInput();
            handler.subscribe(sender, interval, std::move(proxy));

            auto fdlist = ::g_unix_fd_list_new_from_array(&inputFd, 1);
            ::g_dbus_method_invocation_return_value_with_unix_fd_list(
                        invocation,
                        g_variant_new("(h)"),
                        fdlist);

            g_object_unref(fdlist);
        } catch (const PipeException& e) {
            ::g_dbus_method_invocation_return_dbus_error(
                        invocation,
                        "org.tizen.lbs.Providers.FusedLocation.SubscribtionError",
                        e.what());
        }
    }
    else
    {
        LOGW("Received unknown method call.");
        // NOTE: This shouldn't happen with properly defined interface (updated XML)
    }
}

void Server::onAnyNameVanished(const std::string& name)
{
    handler.unregisterClient(name);
}

void Server::registerClient(const Server::ClientId& clientId)
{
    if (!handler.registerClient(clientId))
    {
        return;
    }

    typedef std::pair<Server*, guint> LocalContext;

    auto nameVanishedCb = [] (GDBusConnection*, const gchar* name, gpointer user_data)
    {
        auto pair = static_cast<LocalContext*>(user_data);
        pair->first->onAnyNameVanished(name);

        ::g_bus_unwatch_name(pair->second);
    };

    auto freePair = [] (gpointer ptr)
    {
        delete static_cast<std::pair<Server*, guint>*>(ptr);
    };

    auto context = new LocalContext(this, 0);
    guint ret = ::g_bus_watch_name_on_connection(connection,
                                                 clientId.c_str(),
                                                 G_BUS_NAME_WATCHER_FLAGS_NONE,
                                                 nullptr,
                                                 nameVanishedCb,
                                                 context,
                                                 freePair);

    context->second = ret;
}

namespace
{

bool sendAllData(int fd, const void* data, size_t size)
{
    size_t written = 0;
    const char* ptr = static_cast<const char*>(data);
    while (written < size)
    {
        int ret = ::write(fd, ptr + written, size - written);
        if (ret == 0)
        {
            LOGE("Client communication broken.");
            return false;
        }
        else if (ret < 0)
        {
            LOGE("Client communication error: " + getErrorMessage());
            return false;
        }

        written += ret;
    }

    return true;
}

}

Server::ClientProxy::ClientProxy()
{
    auto ret = ::pipe(fd);
    if (ret != 0)
    {
        const std::string& msg = "Failed to create a pipe for a client: " + getErrorMessage();
        LOGE(msg);
        throw IPCException(msg);
    }
}

int Server::ClientProxy::getInput() const
{
    return fd[0];
}

bool Server::ClientProxy::sendPosition(const ipc::Position& position)
{
    LOGD("Sending an update...");
    return sendAllData(fd[1], &position, sizeof(position));
}

Server::ClientProxy::~ClientProxy()
{
    ::close(fd[0]);
    ::close(fd[1]);
}

}
}
