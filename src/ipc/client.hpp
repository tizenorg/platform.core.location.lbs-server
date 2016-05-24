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

#pragma once

#include "common.hpp"

#include <gio/gio.h>

#include <memory>
#include <functional>

namespace fl
{
namespace ipc
{

class Client final
{
public:
    typedef std::function<void(bool)> ReadyCb;
    typedef std::function<void(const Position&)> PositionCb;

    ~Client();

    void connect(ReadyCb readyCb);
    std::string sendEcho(const std::string& message);
    void setAccuracy(AccuracyLevel level);
    void setPositionCallback(PositionCb positionCb);
    void subscribe();
    void unsubscribe(); // TODO: implement
    bool isConnected() const;

    void onPositionUpdate();

private:
    typedef std::unique_ptr<GDBusNodeInfo, void(*)(GDBusNodeInfo*)> IntrospectionPtr;

    GDBusConnection* connection = nullptr;
    GDBusProxy* serverProxy = nullptr;
    IntrospectionPtr introspection = IntrospectionPtr(nullptr, ::g_dbus_node_info_unref);
    GInputStream* stream = nullptr;
    GSource* source = nullptr;
    PositionCb positionCallback = nullptr;

    void onBusAcquired(ReadyCb readyCb);
    void sourceFd(int fd);
    void registerClient(ReadyCb readyCb);
    bool readAllData(void* buffer, size_t size);
};

}
}