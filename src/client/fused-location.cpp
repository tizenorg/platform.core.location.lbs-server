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
 * @brief   Fused Location CAPI
 */

#include "fused-location.h"

#include "ipc/client.hpp"
#include "ipc/exception.hpp"

#include <glib.h>

#ifndef API
#define API __attribute__((visibility("default")))
#endif

using namespace fl::ipc;

struct fused_location_s {
	Client client;
	Client::ReadyCb readyCb;
};

API int fused_location_set_position_callback(fused_location_h handle,
                                             fused_location_position_updated_cb position_callback,
                                             void* user_data)
{
	if (handle == nullptr) {
		return FUSED_LOCATION_ERROR_INVALID_ARGUMENT;
	}

	if (position_callback == nullptr) {
		handle->client.setPositionCallback(nullptr);
	} else {
		handle->client.setPositionCallback(
		    [position_callback, user_data](const Position& position) {
			    position_callback(position.latitude,
				                  position.longitude,
				                  position.altitude,
				                  0.0, 0.0, 0.0, 0.0, // TODO: provide real data
				                  position.timestamp,
				                  user_data);
		    });
	}

	return FUSED_LOCATION_ERROR_NONE;
}

API int fused_location_set_state_callback(fused_location_h handle,
                                          fused_location_state_updated_cb state_callback,
                                          void* user_data)
{
	if (handle == nullptr) {
		return FUSED_LOCATION_ERROR_INVALID_ARGUMENT;
	}

	if (state_callback == nullptr) {
		handle->readyCb = nullptr;
	} else {
		handle->readyCb = [state_callback, user_data](bool ready) {
			if (ready) {
				state_callback(FUSED_LOCATION_STATE_ENABLED, user_data);
			} else {
				state_callback(FUSED_LOCATION_STATE_DISABLED, user_data);
			}
		};
	}

	return FUSED_LOCATION_ERROR_NONE;
}

API int fused_location_create(fused_location_h* handle)
{
	try {
		*handle = new fused_location_s;
		return FUSED_LOCATION_ERROR_NONE;
	} catch (...) {
		return FUSED_LOCATION_ERROR_ANY;
	}
}

API int fused_location_start(fused_location_h handle)
{
	if (handle == nullptr) {
		return FUSED_LOCATION_ERROR_INVALID_ARGUMENT;
	}

	if (handle->client.isConnected()) {
		return FUSED_LOCATION_ERROR_NONE;
	}

	try {
		handle->client.connect(handle->readyCb);
		handle->client.subscribe();
		return FUSED_LOCATION_ERROR_NONE;
	} catch (...) {
		return FUSED_LOCATION_ERROR_ANY;
	}
}

API int fused_location_stop(fused_location_h handle)
{
	if (handle == nullptr) {
		return FUSED_LOCATION_ERROR_INVALID_ARGUMENT;
	}

	if (!handle->client.isConnected()) {
		return FUSED_LOCATION_ERROR_NOT_READY;
	}

	try {
		handle->client.unsubscribe();
		return FUSED_LOCATION_ERROR_NONE;
	} catch (...) {
		return FUSED_LOCATION_ERROR_ANY;
	}
}

API int fused_location_destroy(fused_location_h handle)
{
	if (handle == nullptr) {
		return FUSED_LOCATION_ERROR_INVALID_ARGUMENT;
	}

	delete handle;
	return FUSED_LOCATION_ERROR_NONE;
}

API int fused_location_set_mode(fused_location_h handle, fused_location_mode_e mode)
{
	if (handle == nullptr) {
		return FUSED_LOCATION_ERROR_INVALID_ARGUMENT;
	}

	if (!handle->client.isConnected()) {
		// TODO: Probably this shouldn't fail, as other methods are available before connection
		return FUSED_LOCATION_ERROR_NOT_READY;
	}

	auto accuracyLevel = static_cast<AccuracyLevel>(mode);

	try {
		handle->client.setAccuracy(accuracyLevel);
	} catch (...) {
		return FUSED_LOCATION_ERROR_ANY;
	}

	return FUSED_LOCATION_ERROR_NONE;
}
