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
 * @brief   IPC server unit tests
 */

#include "Server.h"

#include "ipc/Client.h"
#include "ipc/Exception.h"

#include <cstdio>

namespace fl {
namespace ipc {
namespace test {

std::string TestHandler::echo(const std::string& message)
{
	echoReady.set_value(true);
	return message;
}

void TestHandler::onStart() { onStartReady.set_value(true); }

bool TestHandler::registerClient(const ipc::Server::ClientId&)
{
	registerClientReady.set_value(true);
	return true;
}

bool TestHandler::unregisterClient(const ipc::Server::ClientId&)
{
	unregisterClientReady.set_value(true);
	return true;
}

bool TestHandler::setAccuracyLevel(const Server::ClientId&, const AccuracyLevel&)
{
	setAccuracyLevelReady.set_value(true);
	return true;
}
}
}
}

namespace {

void runCmd(const char* cmd)
{
	auto ret = ::system(cmd);
	ASSERT_NE(ret, 0);
}
}

using namespace fl::ipc;
using namespace fl::ipc::test;

/* Server can be safely instantiated only once for given process */
TEST_F(ServerCommunicationTest, All)
{
	auto startFuture = handler.onStartReady.get_future();
	auto echoFuture = handler.echoReady.get_future();
	auto registerFuture = handler.registerClientReady.get_future();
	auto unregisterFuture = handler.unregisterClientReady.get_future();

	asyncTest([&]() {
		{
			auto status = startFuture.wait_for(std::chrono::seconds(1));
			ASSERT_EQ(status, std::future_status::ready);
		}

		runCmd("dbus-send "
		       "--system "
		       "--type=method_call "
		       "--dest=org.tizen.lbs.Providers.FusedLocation "
		       "/org/tizen/lbs/Providers/FusedLocation "
		       "org.tizen.lbs.Providers.FusedLocation.Server.echo "
		       "\"string:hello\"");

		{
			auto status = echoFuture.wait_for(std::chrono::seconds(1));
			EXPECT_EQ(status, std::future_status::ready);
		}

		runCmd("dbus-send "
		       "--system "
		       "--type=method_call "
		       "--dest=org.tizen.lbs.Providers.FusedLocation "
		       "/org/tizen/lbs/Providers/FusedLocation "
		       "org.tizen.lbs.Providers.FusedLocation.Server.registerClient");

		{
			auto status = registerFuture.wait_for(std::chrono::seconds(1));
			ASSERT_EQ(status, std::future_status::ready);
		}

		{
			auto status = unregisterFuture.wait_for(std::chrono::seconds(1));
			ASSERT_EQ(status, std::future_status::ready);
		}
	});
}
