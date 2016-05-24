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
 * @brief   Common utilities
 */

#pragma once

#include <functional>
#include <memory>
#include <string>

const std::string getErrorMessage();

template <typename R, typename... T> struct CallbackComposer {
public:
	typedef R(CallbackType)(T..., void*);
	typedef std::function<R(T...)> Payload;

	CallbackComposer(CallbackType callback, Payload payload)
	    : callback(callback)
	    , payload(new std::function<R(T...)>(payload))
	{
	}

	CallbackType* getCallback() const { return callback; }

	Payload* getPayload() { return payload.release(); }

private:
	CallbackType* callback;
	std::unique_ptr<Payload> payload;
};

template <typename R, typename... T>
CallbackComposer<R, T...> generateClosureCallback(std::function<R(T...)> closure)
{
	auto callback = [](T... args, void* user_data) {
		auto fn = static_cast<std::function<R(T...)>*>(user_data);
		auto ret = (*fn)(args...);
		delete fn;
		return ret;
	};
	return CallbackComposer<R, T...>(callback, closure);
}

template <typename... T>
CallbackComposer<void, T...> generateClosureCallback(std::function<void(T...)> closure)
{
	auto callback = [](T... args, void* user_data) {
		auto fn = static_cast<std::function<void(T...)>*>(user_data);
		(*fn)(args...);
		delete fn;
	};
	return CallbackComposer<void, T...>(callback, closure);
}

#define PASS_CLOSURE_CALLBACK(C) C.getCallback(), C.getPayload()
