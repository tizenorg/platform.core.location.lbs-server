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
 * @brief   Common constants and types required by IPC client and server
 */

#pragma once

#include "core/types.hpp"

#include <gio/gio.h>

namespace fl
{
namespace ipc
{

typedef core::AccuracyLevel AccuracyLevel;
typedef core::Position Position;

constexpr auto DBUS_PATH_NAME = "/org/tizen/lbs/Providers/FusedLocation";
constexpr auto DBUS_SERVER_SERVICE_NAME = "org.tizen.lbs.Providers.FusedLocation";
constexpr auto DBUS_SERVER_INTERFACE_NAME = "org.tizen.lbs.Providers.FusedLocation.Server";
constexpr auto DBUS_CLIENT_INTERFACE_NAME = "org.tizen.lbs.Providers.FusedLocation.Client";

#ifndef DBUS_SESSION_BUS
constexpr auto DBUS_BUS_TYPE = G_BUS_TYPE_SYSTEM;
#else
constexpr auto DBUS_BUS_TYPE = G_BUS_TYPE_SESSION;
#endif

// TODO: read dynamically
constexpr auto DBUS_INTERFACE_XML = R"(
<node name="/org/tizen/lbs/Providers/FusedLocation">
    <interface name="org.tizen.lbs.Providers.FusedLocation.Server">
        <method name="echo">
            <arg name="message" direction="in" type="s">
                <doc:doc><doc:summary>Message text</doc:summary></doc:doc>
            </arg>
            <arg name="answer" direction="out" type="s">
                <doc:doc><doc:summary>Answer message text</doc:summary></doc:doc>
            </arg>
            <doc:doc>
                <doc:description>
                    <doc:para>
                        Sample echo method.
                    </doc:para>
                </doc:description>
            </doc:doc>
        </method>
        <method name="registerClient">
            <doc:doc>
                <doc:description>
                    <doc:para>
                        Register the client to the service.
                    </doc:para>
                </doc:description>
            </doc:doc>
        </method>
        <method name="setAccuracy">
            <arg name="level" direction="in" type="u">
                <doc:doc><doc:summary>Accuracy level</doc:summary></doc:doc>
            </arg>
            <doc:doc>
                <doc:description>
                    <doc:para>
                        Set desired accuracy.
                    </doc:para>
                </doc:description>
            </doc:doc>
        </method>
        <method name="setDesiredInterval">
            <arg name="interval" direction="in" type="u">
                <doc:doc><doc:summary>Desired interval of the updates</doc:summary></doc:doc>
            </arg>
            <doc:doc>
                <doc:description>
                    <doc:para>
                        Set desired update intervals.
                    </doc:para>
                </doc:description>
            </doc:doc>
        </method>
        <method name="subscribe">
            <arg name="interval" direction="in" type="u">
                <doc:doc><doc:summary>Minimal interval of the updates</doc:summary></doc:doc>
            </arg>
            <arg name="fd" direction="out" type="h">
                <doc:doc><doc:summary>File descriptor receiving updates</doc:summary></doc:doc>
            </arg>
            <doc:doc>
                <doc:description>
                    <doc:para>
                        Returns a file descriptor for receiving position updates.
                    </doc:para>
                </doc:description>
            </doc:doc>
        </method>
    </interface>
</node>
)";

}
}
