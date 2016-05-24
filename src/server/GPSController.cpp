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
 * @file	GPSController.cpp
 * @author  Pawel Kubik (p.kubik@samsung.com)
 * @brief   GPS controller class
 */

#include "GPSController.h"
#include "Exception.h"

#include "common/Logger.h"

namespace fl {

namespace {

// enumeration defined in lbs-server instead of lbs-dbus
constexpr int LBS_STATUS_AVAILABLE = 3;

}

void GPSController::start(const std::chrono::milliseconds& interval)
{
	LOGI("Starting the lbs-server connection...");

	int ret = ::lbs_client_create(LBS_CLIENT_METHOD_GPS, &lbsHandle);
	if (ret != 0) {
		const auto& msg = "Failed to connect lbs-server.";
		LOGE(msg);
		throw ServerException(msg);
	}

	auto signalCb = [](const gchar* signal, GVariant* parameters, gpointer user_data) {
		GPSController* controller = static_cast<GPSController*>(user_data);

		if (!g_strcmp0(signal, "PositionChanged")) {
			controller->onPositionChanged(parameters);
		} else if (!g_strcmp0(signal, "StatusChanged")) {
			controller->onStatusChanged(parameters);
		} else {
			LOGD("Invaild signal " + std::string(signal));
		}
	};

	ret = ::lbs_client_start(lbsHandle, interval.count(), LBS_CLIENT_LOCATION_CB, signalCb, this);
	if (ret != 0) {
		const auto& msg = "Failed to subscribe lbs-server.";
		LOGE(msg);
		throw ServerException(msg);
	}
}

void GPSController::updateInterval(const std::chrono::milliseconds& interval)
{
	LOGI("GPS interval set to " + std::to_string(interval.count()) + "ms");
	int ret = ::lbs_client_set_position_update_interval(lbsHandle, interval.count());
	if (ret != 0) {
		const auto& msg = "Failed to set lbs-server interval.";
		LOGE(msg);
		throw ServerException(msg);
	}
}

GPSController::~GPSController()
{
	if (lbsHandle != nullptr) {
		::lbs_client_destroy(lbsHandle);
	}
}

void GPSController::onPositionChanged(GVariant* parameters)
{
	int method = 0, fields = 0, timestamp = 0;
	double latitude = 0.0, longitude = 0.0, altitude = 0.0, speed = 0.0, direction = 0.0,
	       climb = 0.0;
	GVariant* accuracy = NULL;

	g_variant_get(parameters, "(iiidddddd@(idd))", &method, &fields, &timestamp, &latitude,
	              &longitude, &altitude, &speed, &direction, &climb, &accuracy);

	if (method != LBS_CLIENT_METHOD_GPS) {
		return;
	}

	core::Position position{timestamp, latitude, longitude, altitude};
	updatePosition(position);
}

void GPSController::onStatusChanged(GVariant* parameters)
{
	int status = 0, method = 0;

	g_variant_get(parameters, "(ii)", &method, &status);

	if (method != LBS_CLIENT_METHOD_GPS) {
		return;
	}

	if (status != LBS_STATUS_AVAILABLE) {
		LOGW("Incorrect GPS status.");
	}
}

void GPSController::updatePosition(const core::Position& position)
{
	receiver.updatePosition(position);
}
}
