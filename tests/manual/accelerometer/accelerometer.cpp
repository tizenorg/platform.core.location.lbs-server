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
 * @brief   Accelerometer console test
 */

#include <iostream>
#include <glib.h>
#include "server/AccelerometerController.h"

using namespace fl;

struct TestReceiver : public core::AccelerationReceiver {
	unsigned counter = 0;

	virtual void onAccelerationData(double t, real a[]) override
	{
		if (counter == 0) {
			std::cerr << "Acceleration data: "
			          << t << ", " << a[0] << ", " << a[1] << ", " << a[2]
			          << std::endl;
		}

		++counter;
	}
};

struct AppContext
{
	AccelerometerController controller;
	TestReceiver receiver;
	GMainLoop* loop = g_main_loop_new(nullptr, FALSE);

	void start()
	{
		std::cerr <<"Starting the application..." << std::endl;

		Result result = controller.init();
		if (result != Result::OK) {
			std::cerr << "Failed to initialize the accelerometer." << std::endl;
			stop();
			return;
		}

		std::cerr <<"Starting acceleration updates..." << std::endl;
		result = controller.start(receiver);
		if (result != Result::OK) {
			std::cerr << "Failed to start the updates." << std::endl;
			stop();
			return;
		}

		::g_timeout_add(1000, [](gpointer context) -> gboolean
		{
			auto appContext = static_cast<AppContext*>(context);

			std::cerr << "Received "
			          << appContext->receiver.counter
			          << " new updates."
			          << std::endl;

			appContext->receiver.counter = 0;

			return TRUE;
		}, this);
	}

	void stop()
	{
		::g_main_loop_quit(loop);
	}

	~AppContext()
	{
		::g_main_loop_unref(loop);
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

	std::cerr <<"Starting the main loop..." << std::endl;
	::g_main_loop_run(appContext.loop);

	return 0;
}
