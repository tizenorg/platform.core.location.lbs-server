/*
 * lbs-server
 *
 * Copyright (c) 2010-2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact: Youngae Kang <youngae.kang@samsung.com>, Dong Wei <d.wei@samsung.com>,
 *          Genie Kim <daejins.kim@samsung.com>, Minjune Kim <sena06.kim@samsung.com>,
 *          Ming Zhu <mingwu.zhu@samsung.com>, Congyi Shi <congyi.shi@samsung.com>
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

#include <vconf.h>
#include <location-module.h>

#include <lbs_dbus_client.h>
#include <vconf-internal-location-keys.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "log.h"

#define MAX_NPS_LOC_ITEM	6

typedef struct {
	lbs_client_dbus_h lbs_client;
	gpointer userdata;
	LocModStatusCB status_cb;
	LocModPositionExtCB pos_cb;
	gboolean is_started;
} ModNpsData;

static void status_callback(GVariant *param, void *user_data)
{
	ModNpsData *mod_nps = (ModNpsData *) user_data;
	g_return_if_fail(mod_nps);
	g_return_if_fail(param);
	g_return_if_fail(mod_nps->status_cb);

	int status = 0, method = 0;

	g_variant_get (param, "(ii)", &method, &status);

	MOD_NPS_LOGD("method(%d) status(%d)", method, status);

	if(method != LBS_CLIENT_METHOD_NPS) return;

	if (status == 3) {	//TODO:  LBS_STATUS_AVAILABLE ?
		MOD_NPS_LOGD("LBS_STATUS_AVAILABLE");
		mod_nps->status_cb(TRUE, LOCATION_STATUS_2D_FIX, mod_nps->userdata);
	}
	else {
		MOD_NPS_LOGD("LBS_STATUS_ACQUIRING/ERROR/UNAVAILABLE");
		mod_nps->status_cb(FALSE, LOCATION_STATUS_NO_FIX, mod_nps->userdata);
	}
}

static void position_callback(GVariant *param, void *user_data)
{

	MOD_NPS_LOGD("position_callback");
	ModNpsData *mod_nps = (ModNpsData *)user_data;
	g_return_if_fail(mod_nps);
	g_return_if_fail(mod_nps->pos_cb);

	int method = 0, fields = 0 ,timestamp = 0 , level = 0;
	double latitude = 0.0, longitude = 0.0, altitude = 0.0, speed = 0.0, direction = 0.0, climb = 0.0, horizontal = 0.0, vertical = 0.0;
	GVariant *accuracy = NULL;

	g_variant_get(param, "(iiidddddd@(idd))", &method, &fields, &timestamp, &latitude, &longitude, &altitude, &speed, &direction, &climb, &accuracy);

	if(method != LBS_CLIENT_METHOD_NPS)
		return;

	g_variant_get(accuracy, "(idd)", &level, &horizontal, &vertical);

	LocationPosition *pos = NULL;
	LocationVelocity *vel = NULL;
	LocationAccuracy *acc = NULL;

	if (altitude) {
		pos = location_position_new(timestamp, latitude, longitude, altitude, LOCATION_STATUS_3D_FIX);
	} else {
		pos = location_position_new(timestamp, latitude, longitude, 0.0, LOCATION_STATUS_2D_FIX);
	}

	vel = location_velocity_new(timestamp, speed, direction, climb);
	acc = location_accuracy_new(LOCATION_ACCURACY_LEVEL_DETAILED, horizontal, vertical);
	MOD_NPS_LOGD("time(%d)", pos->timestamp);
	MOD_NPS_LOGD("method(%d)", method);

	mod_nps->pos_cb(TRUE, pos, vel, acc, mod_nps->userdata);

	location_position_free(pos);
	location_velocity_free(vel);
	location_accuracy_free(acc);

}

static void on_signal_callback(const gchar *sig, GVariant *param, gpointer user_data)
{
	if (!g_strcmp0(sig, "PositionChanged")) {
		MOD_NPS_LOGD("PositionChanged");
		position_callback(param, user_data);
	}
	else if (!g_strcmp0(sig, "StatusChanged")) {
		MOD_NPS_LOGD("StatusChanged");
		status_callback(param, user_data);
	}
	else {
		MOD_NPS_LOGD("Invaild signal[%s]", sig);
	}

}

static int start(gpointer handle, LocModStatusCB status_cb, LocModPositionExtCB pos_cb, LocModSatelliteCB sat_cb, gpointer userdata)
{
	MOD_NPS_LOGD("start");
	ModNpsData *mod_nps = (ModNpsData *) handle;
	g_return_val_if_fail(mod_nps, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(status_cb, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(pos_cb, LOCATION_ERROR_NOT_AVAILABLE);

	mod_nps->status_cb = status_cb;
	mod_nps->pos_cb = pos_cb;
	mod_nps->userdata = userdata;

	int ret = LBS_CLIENT_ERROR_NONE;
	ret = lbs_client_create (LBS_CLIENT_METHOD_NPS, &(mod_nps->lbs_client));
	if (ret != LBS_CLIENT_ERROR_NONE || !mod_nps->lbs_client) {
		MOD_NPS_LOGE("Fail to create lbs_client_h. Error[%d]", ret);
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	ret = lbs_client_start (mod_nps->lbs_client, 1, LBS_CLIENT_LOCATION_CB | LBS_CLIENT_LOCATION_STATUS_CB, on_signal_callback, mod_nps);
	if (ret != LBS_CLIENT_ERROR_NONE) {
		if (ret == LBS_CLIENT_ERROR_ACCESS_DENIED) {
			MOD_NPS_LOGE("Access denied[%d]", ret);
			return LOCATION_ERROR_NOT_ALLOWED;
		}

		MOD_NPS_LOGE("Fail to start lbs_client_h. Error[%d]", ret);
		lbs_client_destroy(mod_nps->lbs_client);
		mod_nps->lbs_client = NULL;
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	return LOCATION_ERROR_NONE;
}

static int stop(gpointer handle)
{
	MOD_NPS_LOGD("stop");
	ModNpsData *mod_nps = (ModNpsData *) handle;
	g_return_val_if_fail(mod_nps, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(mod_nps->lbs_client, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(mod_nps->status_cb, LOCATION_ERROR_NOT_AVAILABLE);

	int ret = LBS_CLIENT_ERROR_NONE;

	ret = lbs_client_stop (mod_nps->lbs_client);
	if (ret != LBS_CLIENT_ERROR_NONE) {
		MOD_NPS_LOGE("Fail to stop. Error[%d]", ret);
		lbs_client_destroy(mod_nps->lbs_client);
		mod_nps->lbs_client = NULL;
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	lbs_client_destroy(mod_nps->lbs_client);
	mod_nps->lbs_client = NULL;

	if (mod_nps->status_cb) {
		mod_nps->status_cb (FALSE, LOCATION_STATUS_NO_FIX, mod_nps->userdata);
	}

	mod_nps->status_cb = NULL;
	mod_nps->pos_cb = NULL;

	return LOCATION_ERROR_NONE;
}

static int get_last_position(gpointer handle, LocationPosition ** position, LocationVelocity ** velocity, LocationAccuracy ** accuracy)
{
	MOD_NPS_LOGD("get_last_position");
	ModNpsData *mod_nps = (ModNpsData *) handle;
	g_return_val_if_fail(mod_nps, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(position, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(velocity, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(accuracy, LOCATION_ERROR_PARAMETER);

	int timestamp = 0;
	char location[128] = {0,};
	char *last_location[MAX_NPS_LOC_ITEM] = {0,};
	char *last = NULL;
	char *str = NULL;
	double latitude = 0.0, longitude = 0.0, altitude = 0.0;
	double speed = 0.0, direction = 0.0;
	double hor_accuracy = 0.0;
	int index = 0;
	LocationStatus status = LOCATION_STATUS_NO_FIX;
	LocationAccuracyLevel level = LOCATION_ACCURACY_LEVEL_NONE;

	*position = NULL;
	*velocity = NULL;
	*accuracy = NULL;

	if (vconf_get_int(VCONFKEY_LOCATION_LAST_WPS_TIMESTAMP, &timestamp)) {
		MOD_NPS_LOGD("Error to get VCONFKEY_LOCATION_LAST_WPS_TIMESTAMP");
		return LOCATION_ERROR_NOT_AVAILABLE;
	} else {
		if (timestamp != 0) {
			if (vconf_get_int(VCONFKEY_LOCATION_LAST_WPS_TIMESTAMP, &timestamp) ||
				vconf_get_dbl(VCONFKEY_LOCATION_LAST_WPS_LATITUDE, &latitude) ||
				vconf_get_dbl(VCONFKEY_LOCATION_LAST_WPS_LONGITUDE, &longitude) ||
				vconf_get_dbl(VCONFKEY_LOCATION_LAST_WPS_ALTITUDE, &altitude) ||
				vconf_get_dbl(VCONFKEY_LOCATION_LAST_WPS_SPEED, &speed) ||
				vconf_get_dbl(VCONFKEY_LOCATION_LAST_WPS_DIRECTION, &direction) ||
				vconf_get_dbl(VCONFKEY_LOCATION_LAST_WPS_HOR_ACCURACY, &hor_accuracy)) {
				return LOCATION_ERROR_NOT_AVAILABLE;
			}
		} else {
			if (vconf_get_int(VCONFKEY_LOCATION_NV_LAST_WPS_TIMESTAMP, &timestamp)) {
				return LOCATION_ERROR_NOT_AVAILABLE;
			}
			str = vconf_get_str(VCONFKEY_LOCATION_NV_LAST_WPS_LOCATION);
			if (str == NULL) {
				return LOCATION_ERROR_NOT_AVAILABLE;
			}
			snprintf(location, sizeof(location), "%s", str);
			free(str);

			last_location[index] = (char *)strtok_r(location, ";", &last);
			while (last_location[index++] != NULL) {
				if (index == MAX_NPS_LOC_ITEM)
					break;
				last_location[index] = (char *)strtok_r(NULL, ";", &last);
			}
			if (index != MAX_NPS_LOC_ITEM) {
				return LOCATION_ERROR_NOT_AVAILABLE;
			}
			index = 0;
			latitude = strtod(last_location[index++], NULL);
			longitude = strtod(last_location[index++], NULL);
			altitude = strtod(last_location[index++], NULL);
			speed = strtod(last_location[index++], NULL);
			direction = strtod(last_location[index++], NULL);
			hor_accuracy = strtod(last_location[index], NULL);
		}
	}

	level = LOCATION_ACCURACY_LEVEL_STREET;
	if (timestamp) {
		status = LOCATION_STATUS_3D_FIX;
	} else {
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	*position = location_position_new((guint) timestamp, latitude, longitude, altitude, status);
	*velocity = location_velocity_new((guint) timestamp, speed, direction, 0.0);
	*accuracy = location_accuracy_new(level, hor_accuracy, -1.0);

	return LOCATION_ERROR_NONE;
}

LOCATION_MODULE_API gpointer init(LocModWpsOps * ops)
{
	MOD_NPS_LOGD("init");
	g_return_val_if_fail(ops, NULL);
	ops->start = start;
	ops->stop = stop;
	ops->get_last_position = get_last_position;

	ModNpsData *mod_nps = g_new0(ModNpsData, 1);
	g_return_val_if_fail(mod_nps, NULL);

	mod_nps->userdata = NULL;
	mod_nps->status_cb = NULL;
	mod_nps->pos_cb = NULL;
	mod_nps->is_started = FALSE;

	return mod_nps;
}

LOCATION_MODULE_API void shutdown(gpointer handle)
{
	MOD_NPS_LOGD("shutdown");
	ModNpsData *mod_nps = (ModNpsData *) handle;
	g_return_if_fail(mod_nps);

	if (mod_nps->lbs_client) {
		if (mod_nps->is_started) {
			lbs_client_stop(mod_nps->lbs_client);
		}
		lbs_client_destroy(mod_nps->lbs_client);
		mod_nps->lbs_client = NULL;
	}

	mod_nps->status_cb = NULL;
	mod_nps->pos_cb = NULL;

	g_free(mod_nps);
	mod_nps = NULL;
}
