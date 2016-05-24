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
 * @brief   Server executable entry point
 */

#include "server/Service.h"

using namespace fl;

namespace {

struct Context {
	Service service;
	Result result;
	GMainLoop* loop;
};

}

int main(int /*argc*/, char* /*argv*/ [])
{
	Context mainContext;
	mainContext.loop = g_main_loop_new(nullptr, FALSE);
	if (mainContext.loop == nullptr) {
		LOGE("Failed to create main loop.");
		return -1;
	}

	::g_timeout_add(0, [](gpointer data) -> gboolean
	{
		Context* context = static_cast<Context*>(data);
		context->result = context->service.start();

		if (context->result != Result::OK) {
			LOGE("Failed to start the service.");
			::g_main_loop_quit(context->loop);
		}

		return FALSE;
	}, &mainContext);

	::g_main_loop_run(mainContext.loop);
	::g_main_loop_unref(mainContext.loop);

	if (mainContext.result == Result::OK)
		return 0;
	else
		return -1;
}
