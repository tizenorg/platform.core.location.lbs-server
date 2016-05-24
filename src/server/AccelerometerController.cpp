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
 * @brief   Acceleration controller class
 */

#include "AccelerometerController.h"
#include "common/Logger.h"

namespace fl {

Result AccelerometerController::init()
{
	if (__sensor != nullptr) {
		LOGW("Accelerometer already initialized.");
		return Result::W_ALREADY_INITIALIZED;
	}

	Result result;
	int error = ::sensor_get_default_sensor(SENSOR_ACCELEROMETER, &__sensor);
	if (error == 0) {
		error = ::sensor_create_listener(__sensor, &__listener);
		if (error == 0) {
			result = __setAlwaysOn();
			if (result == Result::OK)
				return Result::OK;

			::sensor_destroy_listener(__listener);
			__listener = nullptr;
		} else
			result = MAKE_RESULT(fl::ResultSeverity::ERROR, fl::ResultOrigin::SENSORS, error);

		::free(__sensor);
		__sensor = nullptr;
	} else
		result = MAKE_RESULT(fl::ResultSeverity::ERROR, fl::ResultOrigin::SENSORS, error);

	LOGE("Accelerometer initialization error.");
	return result;
}

Result AccelerometerController::start(core::AccelerationReceiver& receiver)
{
	auto eventCb = [] (sensor_h, sensor_event_s* event, void* data) {
		auto receiver = static_cast<core::AccelerationReceiver*>(data);

		if (event->value_count != core::SPATIAL_DIMENSION) {
			LOGW("Received " +
			     std::to_string(event->value_count) +
			     " values from the accelerometer.");

			if (event->value_count < core::SPATIAL_DIMENSION) {
				return;
			}
		}

		receiver->onAccelerationData(event->timestamp, event->values);
	};

	int error = ::sensor_listener_set_event_cb(__listener, 100, eventCb, &receiver);
	if (error == 0) {
		error = ::sensor_listener_start(__listener);
		if (error == 0)
			return Result::OK;

		::sensor_listener_unset_event_cb(__listener);
	}

	LOGE("Accelerometer start error.");
	return MAKE_RESULT(fl::ResultSeverity::ERROR, fl::ResultOrigin::SENSORS, error);
}

Result AccelerometerController::stop()
{
	if (__listener) {
		::sensor_listener_stop(__listener);
		::sensor_listener_unset_event_cb(__listener);
	} else {
		LOGW("Accelerometer not initialized.");
		return Result::W_UNINITIALIZED;
	}

	return Result::OK;
}

Result AccelerometerController::shutdown()
{
	if (__listener) {
		::sensor_destroy_listener(__listener);
		__listener = nullptr;
	}

	if (__sensor) {
		::free(__sensor);
		__sensor = nullptr;
	}

	return Result::OK;
}

AccelerometerController::~AccelerometerController()
{
	shutdown();
}

Result AccelerometerController::__setAlwaysOn()
{
	int error = ::sensor_listener_set_option(__listener, SENSOR_OPTION_ALWAYS_ON);
	if (error == 0)
		return Result::OK;

	LOGE("Failed to set 'ALWAYS ON' option.");
	return MAKE_RESULT(fl::ResultSeverity::ERROR, fl::ResultOrigin::SENSORS, error);
}

}
