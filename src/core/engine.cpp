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
 * @brief   Fused Location engine
 */

#include "engine.hpp"

namespace fl
{
namespace core
{

void Engine::setAccuracy(const AccuracyLevel& accuracy)
{
    this->accuracy = accuracy;
}

void Engine::setInterval(const std::chrono::milliseconds& interval)
{
    this->interval = interval;
    intervalReceiver.updateInterval(interval);
}

void Engine::updatePosition(SourceType, const Position& position)
{
    // forward every update
    positionReceiver.updatePosition(position);
}

}
}