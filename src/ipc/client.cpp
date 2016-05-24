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
 * @brief   Client IPC interface
 */

#include "common.hpp"
#include "exception.hpp"
#include "client.hpp"
#include "dbus-error.hpp"

#include "common/logger.hpp"
#include "common/utils.hpp"

#include <gio/gunixfdlist.h>
#include <gio/gunixinputstream.h>

namespace fl
{
namespace ipc
{

namespace
{

bool handleMethodCallError(const Error& error)
{
    if (error.isSet())
    {
        const auto& msg = "Method call failed: " + error.getMessage();
        LOGW(msg);

        auto code = error.getCode();
        if (code == G_IO_ERROR_CLOSED || code == G_IO_ERROR_EXISTS)
        {
            throw ServiceUnavailableException(msg);
        }
        else if (code == G_IO_ERROR_TIMED_OUT)
        {
            throw TimeoutException(msg);
        }
        else
        {
            throw IPCException(msg);
        }
    }

    return true;
}

int getFdFromResult(GUnixFDList* fdList)
{
    Error error;
    int fd = ::g_unix_fd_list_get(fdList, 0, error.pass());
    if (error.isSet())
    {
        const auto& msg = "Method call failed: Failed to read file descriptor";
        LOGW(msg);
        throw IPCException(msg);
    }

    return fd;
}

}

Client::~Client()
{
    if (serverProxy != nullptr)
    {
        ::g_object_unref(serverProxy);
    }
    if (connection != nullptr)
    {
        ::g_object_unref(connection);
    }
    if (source != nullptr)
    {
        ::g_source_destroy(source);
        ::g_source_unref(source);
    }
    if (stream != nullptr)
    {
        ::g_input_stream_close(stream, nullptr, nullptr);
    }
}

void Client::connect(Client::ReadyCb readyCb)
{
    typedef std::function<void(GObject*,GAsyncResult*)> LocalCb;

    LocalCb localCb = [this, readyCb] (GObject*, GAsyncResult* res)
    {
        Error error;

        connection = g_bus_get_finish(res, error.pass());

        if (connection == nullptr)
        {
            const auto msg = "Failed to connect the bus: " + error.getMessage();
            LOGE(msg);
            readyCb(false);
        }

        onBusAcquired(readyCb);
    };

    auto cc = generateClosureCallback(localCb);
    ::g_bus_get(DBUS_BUS_TYPE, nullptr, PASS_CLOSURE_CALLBACK(cc));
}

void Client::onBusAcquired(ReadyCb readyCb)
{
    Error error;

    introspection.reset(::g_dbus_node_info_new_for_xml(DBUS_INTERFACE_XML, error.pass()));

    if (introspection == nullptr)
    {
        const auto& msg = "Failed to parse DBus interface: " + error.getMessage();
        LOGE(msg);
        readyCb(false);
    }

    typedef std::function<void(GObject*,GAsyncResult*)> LocalCb;

    LocalCb localCb = [this, readyCb] (GObject*, GAsyncResult* res)
    {
        Error error;

        serverProxy = ::g_dbus_proxy_new_finish(res, error.pass());

        if (error.isSet())
        {
            const auto& msg = "Failed to create the server proxy: " + error.getMessage();
            LOGE(msg);
            readyCb(false);
        }

        registerClient(readyCb);
    };

    auto cc = generateClosureCallback(localCb);
    ::g_dbus_proxy_new(connection,
                       G_DBUS_PROXY_FLAGS_NONE,
                       introspection->interfaces[0],
                       DBUS_SERVER_SERVICE_NAME,
                       DBUS_PATH_NAME,
                       DBUS_SERVER_INTERFACE_NAME,
                       nullptr,
                       PASS_CLOSURE_CALLBACK(cc));
}

std::string Client::sendEcho(const std::string& message)
{
    LOGI("Sending an echo: " + message);

    Error error;
    GVariant* result = ::g_dbus_proxy_call_sync(serverProxy,
                                                "echo",
                                                g_variant_new ("(s)", message.c_str()),
                                                G_DBUS_CALL_FLAGS_NONE,
                                                1000,
                                                nullptr,
                                                error.pass());

    handleMethodCallError(error);

    const gchar* answer;
    ::g_variant_get(result, "(&s)", &answer);

    return answer;
}

void Client::registerClient(ReadyCb readyCb)
{
    LOGI("Registering the client...");

    Error error;
    ::g_dbus_proxy_call_sync(serverProxy,
                             "registerClient",
                             g_variant_new ("()"),
                             G_DBUS_CALL_FLAGS_NONE,
                             10000,
                             nullptr,
                             error.pass());

    readyCb(handleMethodCallError(error));
}

void Client::setAccuracy(AccuracyLevel level)
{
    LOGI("Setting desired accuracy...");

    Error error;
    ::g_dbus_proxy_call_sync(serverProxy,
                             "setAccuracy",
                             g_variant_new ("(u)", level),
                             G_DBUS_CALL_FLAGS_NONE,
                             10000,
                             nullptr,
                             error.pass());

    handleMethodCallError(error);
}

void Client::setPositionCallback(Client::PositionCb positionCb)
{
    positionCallback = positionCb;
}

void Client::unsubscribe()
{
    LOGW("Unsubscribe is not implemented yet!");
}

bool Client::isConnected() const
{
    return connection != nullptr;
}

namespace
{

gboolean onUpdate(GObject*, gpointer data)
{
    LOGI("Received update.");

    Client* client = static_cast<Client*>(data);
    client->onPositionUpdate();

    return TRUE;
}

}

void Client::sourceFd(int fd)
{
    stream = ::g_unix_input_stream_new(fd, TRUE);
    source = ::g_pollable_input_stream_create_source(
                reinterpret_cast<GPollableInputStream*>(stream),
                nullptr);

    ::g_source_set_callback(source, reinterpret_cast<GSourceFunc>(onUpdate), this, nullptr);
    ::g_source_attach(source, g_main_context_default());
}

void Client::subscribe()
{
    LOGI("Subscribing to position updates...");

    unsigned interval = 1000; // NOTE: interval passing will be removed in the future

    Error error;
    GUnixFDList* fdList;
    ::g_dbus_proxy_call_with_unix_fd_list_sync(serverProxy,
                                               "subscribe",
                                               g_variant_new ("(u)", interval),
                                               G_DBUS_CALL_FLAGS_NONE,
                                               10000,
                                               nullptr,
                                               &fdList,
                                               nullptr,
                                               error.pass());

    handleMethodCallError(error);
    int fd = getFdFromResult(fdList);

    sourceFd(fd);
}

bool Client::readAllData(void* buffer, size_t size)
{
    Error error;
    size_t read = 0;
    char* ptr = static_cast<char*>(buffer);
    while (read < size)
    {
        gssize ret = ::g_pollable_stream_read(stream,
                                        ptr + read,
                                        size - read,
                                        false,
                                        nullptr,
                                        error.pass());

        if (ret == 0)
        {
            const auto& msg = "Server communication broken.";
            LOGE(msg);
            throw IPCException(msg);
        }
        else if (ret < 0)
        {
            if (errno == G_IO_ERROR_WOULD_BLOCK && read == 0)
            {
                return false;
            }

            const auto& msg = "Server communication error: " + getErrorMessage();
            LOGE(msg);
            throw IPCException(msg);
        }

        read += ret;
    }

    return true;
}

void Client::onPositionUpdate()
{
    Position position;

    if (readAllData(&position, sizeof(position)))
    {
        if (positionCallback != nullptr)
        {
            positionCallback(position);
        }
    }
    // else spurious wakeup - ignore
}

}
}
