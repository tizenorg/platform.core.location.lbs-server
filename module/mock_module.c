/*
 * lbs-server
 *
 * Copyright (c) 2011-2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Youngae Kang <youngae.kang@samsung.com>, Minjune Kim <sena06.kim@samsung.com>
 *          Genie Kim <daejins.kim@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vconf.h>
#include <vconf-internal-location-keys.h>
#include <location-module.h>

#include <dlfcn.h>
#include <lbs_dbus_client.h>
#include <lbs_agps.h>

#include "log.h"

#define MAX_GPS_LOC_ITEM	7

typedef struct {
	lbs_client_dbus_h lbs_client;
	LocModStatusCB mock_status_cb;
	LocModStatusCB status_cb;
	LocModPositionExtCB pos_cb;
	gpointer userdata;
	gboolean is_started;
	LocationPosition *last_pos;
	LocationVelocity *last_vel;
	LocationAccuracy *last_acc;
} ModMockData;

typedef enum {
	LBS_STATUS_ERROR, //from lbs-server
	LBS_STATUS_UNAVAILABLE, //from lbs-server
	LBS_STATUS_ACQUIRING,  // from lbs-server
	LBS_STATUS_AVAILABLE, // from lbs-server
	LBS_STATUS_BATCH, //from lbs-server
	LBS_STATUS_MOCK_SET, //from lbs-dbus status for mock location
	LBS_STATUS_MOCK_FAIL, // from lbs-dbus status fro mock location
} LbsStatus;

static void __status_callback(GVariant *param, void *user_data)
{
	ModMockData *mod_mock = (ModMockData *) user_data;
	g_return_if_fail(param);
	g_return_if_fail(mod_mock);
	g_return_if_fail(mod_mock->status_cb);

	int status = 0, method = 0;

	g_variant_get(param, "(ii)", &method, &status);

	MOD_MOCK_LOGD("method(%d) status(%d)", method, status);

	if (method != LBS_CLIENT_METHOD_MOCK) return;

	if (status == LBS_STATUS_AVAILABLE) {	/*TODO: LBS_STATUS_AVAILABLE ? */
		MOD_MOCK_LOGD("LBS_STATUS_AVAILABLE");
		mod_mock->status_cb(TRUE, LOCATION_STATUS_3D_FIX, mod_mock->userdata);
	} else {
		MOD_LOGD("LBS_STATUS_ACQUIRING/ERROR/UNAVAILABLE. Status[%d]", status);
		mod_mock->status_cb(FALSE, LOCATION_STATUS_NO_FIX, mod_mock->userdata);
	}
}

static void __position_callback(GVariant *param, void *user_data)
{
	MOD_MOCK_LOGD("position_callback");
	ModMockData *mod_mock = (ModMockData *)user_data;
	g_return_if_fail(mod_mock);
	g_return_if_fail(mod_mock->pos_cb);

	int method = 0, fields = 0 , timestamp = 0 , level = 0;
	double latitude = 0.0, longitude = 0.0, altitude = 0.0, speed = 0.0, direction = 0.0, climb = 0.0, horizontal = 0.0, vertical = 0.0;
	GVariant *accuracy = NULL;

	g_variant_get(param, "(iiidddddd@(idd))", &method, &fields, &timestamp, &latitude, &longitude, &altitude, &speed, &direction, &climb, &accuracy);

	if (method != LBS_CLIENT_METHOD_MOCK)
		return;

	g_variant_get(accuracy, "(idd)", &level, &horizontal, &vertical);

	LocationPosition *pos = NULL;
	LocationVelocity *vel = NULL;
	LocationAccuracy *acc = NULL;

	pos = location_position_new(timestamp, latitude, longitude, altitude, LOCATION_STATUS_3D_FIX);

	vel = location_velocity_new(timestamp, speed, direction, climb);

	acc = location_accuracy_new(LOCATION_ACCURACY_LEVEL_DETAILED, horizontal, vertical);

	mod_mock->pos_cb(TRUE, pos, vel, acc, mod_mock->userdata);
	mod_mock->last_pos = location_position_copy(pos);
	mod_mock->last_vel = location_velocity_copy(vel);
	mod_mock->last_acc = location_accuracy_copy(acc);

	location_position_free(pos);
	location_velocity_free(vel);
	location_accuracy_free(acc);
	g_variant_unref(accuracy);
}

static void __on_signal_callback(const gchar *sig, GVariant *param, gpointer user_data)
{
	if (!g_strcmp0(sig, "PositionChanged")) {
		__position_callback(param, user_data);
	} else if (!g_strcmp0(sig, "StatusChanged")) {
		__status_callback(param, user_data);
	} else {
		MOD_MOCK_LOGD("Invaild signal[%s]", sig);
	}
}

//static int start(gpointer handle, guint pos_update_interval, LocModStatusCB status_cb, LocModPositionExtCB pos_cb, gpointer userdata)
static int start(gpointer handle, LocModStatusCB status_cb, LocModPositionExtCB pos_cb, gpointer userdata)
{
	MOD_MOCK_LOGD("ENTER >>>");
	ModMockData *mod_mock = (ModMockData *) handle;
	g_return_val_if_fail(mod_mock, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(status_cb, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(pos_cb, LOCATION_ERROR_NOT_AVAILABLE);

	mod_mock->status_cb = status_cb;
	mod_mock->pos_cb = pos_cb;
	mod_mock->userdata = userdata;

	int ret = LBS_CLIENT_ERROR_NONE;
	if (mod_mock->lbs_client == NULL) {
		ret = lbs_client_create(LBS_CLIENT_METHOD_MOCK , &(mod_mock->lbs_client));
		if (ret != LBS_CLIENT_ERROR_NONE || !mod_mock->lbs_client) {
			MOD_MOCK_LOGE("Fail to create lbs_client_h. Error[%d]", ret);
			return LOCATION_ERROR_NOT_AVAILABLE;
		}
	}
	MOD_MOCK_LOGD("mod_mock(%p)", mod_mock);
	MOD_MOCK_LOGD("pos_cb(%p), user_data(%p)", mod_mock->pos_cb, mod_mock->userdata);

	ret = lbs_client_start(mod_mock->lbs_client, 1, LBS_CLIENT_LOCATION_CB | LBS_CLIENT_LOCATION_STATUS_CB, __on_signal_callback, mod_mock);
	if (ret != LBS_CLIENT_ERROR_NONE) {
		if (ret == LBS_CLIENT_ERROR_ACCESS_DENIED) {
			MOD_MOCK_LOGE("Access denied[%d]", ret);
			return LOCATION_ERROR_NOT_ALLOWED;
		}
		MOD_MOCK_LOGE("Fail to start lbs_client_h. Error[%d]", ret);
		lbs_client_destroy(mod_mock->lbs_client);
		mod_mock->lbs_client = NULL;

		return LOCATION_ERROR_NOT_AVAILABLE;
	}
	MOD_MOCK_LOGD("EXIT <<<");

	return LOCATION_ERROR_NONE;
}

static int stop(gpointer handle)
{
	MOD_MOCK_LOGD("stop");
	ModMockData *mod_mock = (ModMockData *) handle;
	g_return_val_if_fail(mod_mock, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(mod_mock->lbs_client, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(mod_mock->status_cb, LOCATION_ERROR_NOT_AVAILABLE);

	int ret = LBS_CLIENT_ERROR_NONE;

	ret = lbs_client_stop(mod_mock->lbs_client);
	if (ret != LBS_CLIENT_ERROR_NONE) {
		MOD_MOCK_LOGE("Fail to stop. Error[%d]", ret);
		lbs_client_destroy(mod_mock->lbs_client);
		mod_mock->lbs_client = NULL;
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	ret = lbs_client_destroy(mod_mock->lbs_client);
	if (ret != LBS_CLIENT_ERROR_NONE) {
		MOD_MOCK_LOGE("Fail to destroy. Error[%d]", ret);
		return LOCATION_ERROR_NOT_AVAILABLE;
	}
	mod_mock->lbs_client = NULL;

	if (mod_mock->status_cb) {
		mod_mock->status_cb(FALSE, LOCATION_STATUS_NO_FIX, mod_mock->userdata);
	}
	mod_mock->status_cb = NULL;
	mod_mock->pos_cb = NULL;

	return LOCATION_ERROR_NONE;
}

static int get_last_position(gpointer handle, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy)
{
	MOD_MOCK_LOGD("get_last_position");
	ModMockData *mod_mock = (ModMockData *) handle;
	g_return_val_if_fail(mod_mock, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(position, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(velocity, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(accuracy, LOCATION_ERROR_PARAMETER);

	*position = NULL;
	*velocity = NULL;
	*accuracy = NULL;

	if (mod_mock->last_pos)
		*position = location_position_copy(mod_mock->last_pos);
	if (mod_mock->last_vel)
		*velocity = location_velocity_copy(mod_mock->last_vel);
	if (mod_mock->last_acc) {
		*accuracy = location_accuracy_copy(mod_mock->last_acc);
	}

	return LOCATION_ERROR_NONE;
}

static int set_option(gpointer handle, const char *option)
{
	ModMockData *mod_mock = (ModMockData *) handle;
	g_return_val_if_fail(mod_mock, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(option, LOCATION_ERROR_PARAMETER);
	MOD_MOCK_LOGD("set_option : %s", option);

	int ret = LBS_CLIENT_ERROR_NONE;
	ret = lbs_set_option(option);
	if (ret != LBS_AGPS_ERROR_NONE) {
		MOD_MOCK_LOGE("Fail to lbs_set_option [%d]", ret);
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	return LOCATION_ERROR_NONE;
}

#if 0
static int set_position_update_interval(gpointer handle, guint interval)
{
	MOD_LOGD("set_position_update_interval [%d]", interval);
	ModMockData *mod_mock = (ModMockData *) handle;
	g_return_val_if_fail(mod_mock, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(mod_mock->lbs_client, LOCATION_ERROR_NOT_AVAILABLE);

	int ret;
	ret = lbs_client_set_position_update_interval(mod_mock->lbs_client, interval);
	if (ret != LBS_CLIENT_ERROR_NONE) {
		if (ret == LBS_CLIENT_ERROR_ACCESS_DENIED) {
			MOD_LOGE("Access denied[%d]", ret);
			return LOCATION_ERROR_NOT_ALLOWED;
		}
		MOD_LOGE("Failed lbs_client_set_position_update_interval. Error[%d]", ret);
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	return LOCATION_ERROR_NONE;
}
#endif


/* Tizen 3.0 Mock Location */

static void __on_set_mock_location_callback(const gchar *sig, GVariant *param, gpointer userdata)
{
	ModMockData *mod_mock = (ModMockData *) userdata;
	g_return_if_fail(param);
	g_return_if_fail(mod_mock);
	g_return_if_fail(mod_mock);

	MOD_MOCK_LOGD("ENTER >>>");

	if (g_strcmp0(sig, "SetLocation")) {
		MOD_MOCK_LOGI("Invaild signal[%s]", sig);
		return;
	}

	int method = 0, status = 0;

	g_variant_get(param, "(ii)", &method, &status);
	MOD_MOCK_LOGD("method(%d) status(%d)", method, status);

	if (method != LBS_CLIENT_METHOD_MOCK) {
		MOD_MOCK_LOGI("Invaild method(%d)", method);
	}

	if (status == LBS_STATUS_MOCK_FAIL) {	/* LBS_STATUS_BATCH + 2 */
		MOD_MOCK_LOGD("LBS_STATUS_MOCK_FAIL");
		mod_mock->mock_status_cb(FALSE, LOCATION_STATUS_MOCK_FAIL, mod_mock->userdata);
	} else {
		MOD_MOCK_LOGD("Others. Status[%d]", status); /* Nothing to do */
	}
	MOD_MOCK_LOGD("EXIT <<<");

	return;
}

static int set_mock_location(gpointer handle, LocationPosition *position, LocationVelocity *velocity, LocationAccuracy *accuracy,
	LocModStatusCB mock_status_cb, gpointer userdata)
{
	MOD_MOCK_LOGD("ENTER >>>");
	ModMockData *mod_mock = (ModMockData *) handle;
	g_return_val_if_fail(mod_mock, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(mock_status_cb, LOCATION_ERROR_NOT_AVAILABLE);

	int ret = LBS_CLIENT_ERROR_NONE;

	if (mod_mock->lbs_client == NULL) {
		ret = lbs_client_create(LBS_CLIENT_METHOD_MOCK, &(mod_mock->lbs_client));
		if (ret != LBS_CLIENT_ERROR_NONE || !mod_mock->lbs_client) {
			MOD_MOCK_LOGE("Fail to create lbs_client_h. Error[%d]", ret);
			return LOCATION_ERROR_NOT_AVAILABLE;
		}
	}
	MOD_MOCK_LOGD("mod_mock(%p)", mod_mock);

	mod_mock->mock_status_cb = mock_status_cb;
	mod_mock->userdata = userdata;

	ret = lbs_client_set_mock_location_async(mod_mock->lbs_client, LBS_CLIENT_METHOD_MOCK, position->latitude, position->longitude, position->altitude,
		velocity->speed, velocity->direction, accuracy->horizontal_accuracy,
		__on_set_mock_location_callback, mod_mock);

	if (ret != LBS_CLIENT_ERROR_NONE) {
		if (ret == LBS_CLIENT_ERROR_ACCESS_DENIED) {
			MOD_MOCK_LOGE("Access denied[%d]", ret);
			return LOCATION_ERROR_NOT_ALLOWED;
		}
		MOD_MOCK_LOGE("Fail to start lbs_client_h. Error[%d]", ret);
		lbs_client_destroy(mod_mock->lbs_client);
		mod_mock->lbs_client = NULL;

		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	return LOCATION_ERROR_NONE;
}


LOCATION_MODULE_API gpointer init(LocModMockOps *ops)
{
	MOD_MOCK_LOGD("init");

	g_return_val_if_fail(ops, NULL);
	ops->start = start;
	ops->stop = stop;
	ops->get_last_position = get_last_position;
	ops->set_option = set_option;

	/*
	ops->start_batch = NULL;
	ops->stop_batch = NULL;
	ops->set_position_update_interval = set_position_update_interval;
	*/

	/* Tien 3.0 */
	ops->set_mock_location = set_mock_location;

	ModMockData *mod_mock = g_new0(ModMockData, 1);
	g_return_val_if_fail(mod_mock, NULL);

	return (gpointer) mod_mock;
}

LOCATION_MODULE_API void shutdown(gpointer handle)
{
	MOD_MOCK_LOGD("shutdown");
	g_return_if_fail(handle);
	ModMockData *mod_mock = (ModMockData *) handle;
	if (mod_mock->lbs_client) {
		lbs_client_stop(mod_mock->lbs_client);
		lbs_client_destroy(mod_mock->lbs_client);
		mod_mock->lbs_client = NULL;
	}

	mod_mock->status_cb = NULL;
	mod_mock->pos_cb = NULL;
	/*
	mod_mock->batch_cb = NULL;
	*/
	if (mod_mock->last_pos) location_position_free(mod_mock->last_pos);
	if (mod_mock->last_vel) location_velocity_free(mod_mock->last_vel);
	if (mod_mock->last_acc) location_accuracy_free(mod_mock->last_acc);

	mod_mock->last_pos = NULL;
	mod_mock->last_vel = NULL;
	mod_mock->last_acc = NULL;

	g_free(mod_mock);
	mod_mock = NULL;
}
