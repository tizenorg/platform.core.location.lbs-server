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
 * @brief   Service main class
 */

#include "Service.h"
#include "Exception.h"

#include "common/Logger.h"

namespace fl {

Service::Service()
    : gpsController(*this)
    , engine(server, gpsController)
    , server(engine)
{
}

Result Service::start()
{
    LOGI("Starting the server...");
    try {
        gpsController.start();

        Result result = accelerometerController.init();
        if (result == Result::OK) {
            return accelerometerController.start(engine.getAccelerationReceiver());
        }

        return result;
    } catch (const ServerException&) {
        return Result::E_FAILURE;
    }
}

void Service::updatePosition(const core::Position& position)
{
    engine.updatePosition(core::SourceType::GPS, position);
}
}
