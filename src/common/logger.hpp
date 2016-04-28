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
 * @brief   Simple logger
 */

#pragma once

#include <iostream> // NOTE: This is temporary solution
#include <string>

namespace fl
{
namespace logger
{
namespace
{

enum LogLevel : std::uint8_t
{
    LOG_ERROR = 0,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG
};

char LOG_LEVEL_SYMBOLS[] = {'E', 'W', 'I', 'D'};

void log(LogLevel level,
         const std::string& message,
         const char* file,
         unsigned line)
{
    std::cerr << ":: " << LOG_LEVEL_SYMBOLS[level]
              << " :: " << file
              << " : " << line
              << " -:\t" << message
              << std::endl;
}

}
}
}

#define LOG(LEVEL, MESSAGE) { fl::logger::log(LEVEL, MESSAGE, __FILE__, __LINE__); }

#define LOGE(MESSAGE) LOG(fl::logger::LOG_ERROR, MESSAGE)
#define LOGW(MESSAGE) LOG(fl::logger::LOG_WARNING, MESSAGE)
#define LOGI(MESSAGE) LOG(fl::logger::LOG_INFO, MESSAGE)
#define LOGD(MESSAGE) LOG(fl::logger::LOG_DEBUG, MESSAGE)
