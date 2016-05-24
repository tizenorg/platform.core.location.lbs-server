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
 * @brief   DBusError wrapper
 */

#pragma once

#include <gio/gio.h>
#include <string>

namespace fl {
namespace ipc {

class Error final {
	GError* error = nullptr;

public:
	GError** pass()
	{
		clear();
		return &error;
	}

	bool isSet() const { return error != nullptr; }

	void clear() { ::g_clear_error(&error); }

	int getCode() const
	{
		g_assert(error != nullptr);
		return error->code;
	}

	std::string getMessage() const
	{
		g_assert(error != nullptr);
		return error->message;
	}

	Error() { clear(); }

	~Error()
	{
		if (error != nullptr) {
			::g_error_free(error);
		}
	}
};
}
}
