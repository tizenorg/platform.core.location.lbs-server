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
 * @brief   Server IPC test classes
 */

#pragma once

#include "ipc/server.hpp"

#include <gtest/gtest.h>
#include <thread>
#include <future>

#include "common/logger.hpp"

namespace fl
{
namespace ipc
{
namespace test
{

struct GApplication
{
    GMainLoop* loop = ::g_main_loop_new(nullptr, FALSE);

    void start()
    {
        ::g_main_loop_run(loop);
    }

    void stop()
    {
        ::g_main_loop_quit(loop);
    }

    ~GApplication()
    {
        ::g_main_loop_unref(loop);
    }
};

class TestHandler : public ipc::Server::Handler
{
public:
    std::promise<bool> echoReady;
    std::promise<bool> onStartReady;
    std::promise<bool> registerClientReady;
    std::promise<bool> unregisterClientReady;
    std::promise<bool> setAccuracyLevelReady;

    virtual std::string echo(const std::string& message);
    virtual void onStart();
    virtual bool registerClient(const ipc::Server::ClientId&);
    virtual bool unregisterClient(const ipc::Server::ClientId&);
    virtual bool setAccuracyLevel(const ipc::Server::ClientId&, const AccuracyLevel&);
    virtual bool setDesiredInterval(const ipc::Server::ClientId&, const unsigned) { return true; }
    virtual bool subscribe(const Server::ClientId&,
                           const unsigned,
                           std::unique_ptr<Server::ClientProxy>&&) { return true; }
};

struct ServerMock : GApplication
{
    TestHandler handler;
    ipc::Server server;

    ServerMock() :
        server(handler)
    {}
};

struct ServerCommunicationTest : public ServerMock, public ::testing::Test
{
    /**
     * In separate thread, waits untill specified condition is met and runs the function.
     */
    template <typename T>
    void asyncTest(T&& function)
    {
        std::thread thread = std::thread([&]
        {
            function();
            stop();
        });

        start();

        thread.join();
    }
};

}
}
}
