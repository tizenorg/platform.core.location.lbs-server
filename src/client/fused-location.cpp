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
 * @brief   Fused Location CAPI
 */

#include "fused-location.h"

#include "ipc/client.hpp"
#include "ipc/exception.hpp"

#include <glib.h>

#ifndef API
#define API __attribute__((visibility("default")))
#endif

using namespace fl::ipc;

namespace
{

Client& globalClient()
{
    static Client client;
    return client;
}

}

int fused_location_start(fused_location_position_updated_cb position_callback,
                         void* user_data)
{
    if (position_callback == nullptr)
    {
        return FUSED_LOCATION_ERROR_ANY;
    }

    Client& client = globalClient();

    try
    {
        client.subscribe(2000, [position_callback, user_data] (const Position& position)
        {
            position_callback(position.latitude,
                              position.longitude,
                              position.altitude,
                              position.timestamp,
                              user_data);
        });
        return FUSED_LOCATION_ERROR_NONE;
    }
    catch (...)
    {
        return FUSED_LOCATION_ERROR_ANY;
    }
}

int fused_location_stop()
{
    Client& client = globalClient();
    try {
        client.unsubscribe();
        return FUSED_LOCATION_ERROR_NONE;
    } catch (...) {
        return FUSED_LOCATION_ERROR_ANY;
    }
}

int fused_location_set_mode(fused_location_mode_e mode)
{
    Client& client = globalClient();
    try
    {
        client.setAccuracy(static_cast<AccuracyLevel>(mode));
        return FUSED_LOCATION_ERROR_NONE;
    }
    catch (...)
    {
        return FUSED_LOCATION_ERROR_ANY;
    }
}
