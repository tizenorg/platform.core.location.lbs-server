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
 * @brief   Client executable entry point
 *
 * TODO: Remove this temporary test file after implementing proper unit tests
 */

#include "ipc/client.hpp"
#include "ipc/exception.hpp"

#include "common/logger.hpp"

#include <ctime>

using namespace fl;

struct AppContext
{
    ipc::Client client;
    GMainLoop* loop = g_main_loop_new(nullptr, FALSE);

    void start()
    {
        LOGI("Starting the client...");

        try
        {
            client.registerClient();
            LOGI("Client registered.");

            client.setAccuracy(ipc::AccuracyLevel::HIGH_ACCURACY);
            client.setDesiredInterval(2000);
            client.subscribe(2000, [] (const ipc::Position& position)
            {
                LOGI("Position update: " + std::to_string(position.timestamp) + ", "
                                         + std::to_string(position.latitude) + ", "
                                         + std::to_string(position.longitude) + ", "
                                         + std::to_string(position.altitude));

            });
        }
        catch (const ipc::TimeoutException&)
        {
            LOGW("Service unavailable.");
            ::g_main_loop_quit(loop);
        }

        ::g_timeout_add(4000, [](gpointer user_data) -> gboolean
        {
            try
            {
                auto context = static_cast<AppContext*>(user_data);

                context->client.setAccuracy(static_cast<ipc::AccuracyLevel>(std::time(0) % 3));
                context->client.setDesiredInterval((std::time(0) % 10 + 1) * 1000);
            }
            catch (const ipc::TimeoutException&)
            {
                LOGW("Service unavailable.");
            }

            return TRUE;
        }, this);
    }
};

int main(int /*argc*/, char* /*argv*/[])
{
    AppContext appContext;

    ::g_timeout_add(0, [](gpointer context) -> gboolean
    {
        static_cast<AppContext*>(context)->start();
        return FALSE;
    }, &appContext);

    ::g_main_loop_run(appContext.loop);

    return 0;
}
