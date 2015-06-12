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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "log.h"

#define MAX_GPS_LOC_ITEM	7

typedef struct {
	lbs_client_dbus_h lbs_client;
	LocModStatusCB status_cb;
	LocModPositionExtCB pos_cb;
	LocModBatchExtCB batch_cb;
	LocModSatelliteCB sat_cb;
	gpointer userdata;
	gboolean is_started;
} GpsManagerData;

static void status_callback(GVariant *param, void *user_data)
{
	GpsManagerData *gps_manager = (GpsManagerData *) user_data;
	g_return_if_fail(param);
	g_return_if_fail(gps_manager);
	g_return_if_fail(gps_manager->status_cb);

	int status = 0, method = 0;

	g_variant_get(param, "(ii)", &method, &status);

	MOD_LOGD("method(%d) status(%d)", method, status);

	if (method != LBS_CLIENT_METHOD_GPS) return;

	if (status == 3) {	/*TODO: LBS_STATUS_AVAILABLE ? */
		MOD_LOGD("LBS_STATUS_AVAILABLE");
		gps_manager->status_cb(TRUE, LOCATION_STATUS_3D_FIX, gps_manager->userdata);
	} else {
		MOD_LOGD("LBS_STATUS_ACQUIRING/ERROR/UNAVAILABLE. Status[%d]", status);
		gps_manager->status_cb(FALSE, LOCATION_STATUS_NO_FIX, gps_manager->userdata);
	}
}

static void satellite_callback(GVariant *param, void *user_data)
{
	GpsManagerData *gps_manager = (GpsManagerData *)user_data;
	g_return_if_fail(gps_manager);
	g_return_if_fail(gps_manager->sat_cb);

	guint idx;
	guint used_idx;
	guint *used_prn_array = NULL;
	gboolean ret = FALSE;
	int timestamp = 0, satellite_used = 0, satellite_visible = 0;

	LocationSatellite *sat = NULL;
	GVariant *used_prn = NULL;
	GVariantIter *used_prn_iter = NULL;
	GVariant *sat_info = NULL;
	GVariantIter *sat_iter = NULL;
	int prn = 0, elev = 0, azim = 0, snr = 0;

	g_variant_get(param, "(iii@ai@a(iiii))", &timestamp, &satellite_used, &satellite_visible, &used_prn, &sat_info);
	g_variant_get(used_prn, "ai", &used_prn_iter);
	g_variant_get(sat_info, "a(iiii)", &sat_iter);
	MOD_LOGD("timestamp [%d], satellite_used [%d], satellite_visible[%d]", timestamp, satellite_used, satellite_visible);
	int tmp_prn = 0;
	int num_of_used_prn = g_variant_iter_n_children(used_prn_iter);
	if (num_of_used_prn > 0) {
		used_prn_array = (guint *)g_new0(guint, num_of_used_prn);
		for (idx = 0; idx < num_of_used_prn; idx++) {
			ret = g_variant_iter_next(used_prn_iter, "i", &tmp_prn);
			if (ret == FALSE)
				break;
			used_prn_array[idx] = tmp_prn;
		}
	}
	sat = location_satellite_new(satellite_visible);

	sat->timestamp = timestamp;
	sat->num_of_sat_inview = satellite_visible;
	sat->num_of_sat_used = satellite_used;

	GVariant *tmp_var = NULL;
	for (idx = 0; idx < satellite_visible; idx++) {
		gboolean used = FALSE;
		tmp_var = g_variant_iter_next_value(sat_iter);
		g_variant_get(tmp_var, "(iiii)", &prn, &elev, &azim, &snr);
		if (used_prn_array != NULL) {
			for (used_idx = 0; used_idx < satellite_used; used_idx++) {
				if (prn == used_prn_array[used_idx]) {
					used = TRUE;
					break;
				}
			}
		}
		/*MOD_LOGD("prn[%d] : used %d elev %d azim %d snr %d", prn, used, elev, azim, snr); */
		location_satellite_set_satellite_details(sat, idx, prn, used, elev, azim, snr);
		g_variant_unref(tmp_var);
	}

	gps_manager->sat_cb(TRUE, sat, gps_manager->userdata);
	location_satellite_free(sat);
	g_variant_iter_free(used_prn_iter);
	g_variant_iter_free(sat_iter);
	g_variant_unref(used_prn);
	g_variant_unref(sat_info);


	if (used_prn_array) {
		g_free(used_prn_array);
		used_prn_array = NULL;
	}
}

#if 0
static void nmea_callback(GeoclueNmea *nmea, int timestamp, char *data, gpointer userdata)
{
}
#endif

static void position_callback(GVariant *param, void *user_data)
{
	MOD_LOGD("position_callback");
	GpsManagerData *gps_manager = (GpsManagerData *)user_data;
	g_return_if_fail(gps_manager);
	g_return_if_fail(gps_manager->pos_cb);

	int method = 0, fields = 0 , timestamp = 0 , level = 0;
	double latitude = 0.0, longitude = 0.0, altitude = 0.0, speed = 0.0, direction = 0.0, climb = 0.0, horizontal = 0.0, vertical = 0.0;
	GVariant *accuracy = NULL;

	g_variant_get(param, "(iiidddddd@(idd))", &method, &fields, &timestamp, &latitude, &longitude, &altitude, &speed, &direction, &climb, &accuracy);

	if (method != LBS_CLIENT_METHOD_GPS)
		return;

	g_variant_get(accuracy, "(idd)", &level, &horizontal, &vertical);

	LocationPosition *pos = NULL;
	LocationVelocity *vel = NULL;
	LocationAccuracy *acc = NULL;

	pos = location_position_new(timestamp, latitude, longitude, altitude, LOCATION_STATUS_3D_FIX);

	vel = location_velocity_new(timestamp, speed, direction, climb);

	acc = location_accuracy_new(LOCATION_ACCURACY_LEVEL_DETAILED, horizontal, vertical);

	gps_manager->pos_cb(TRUE, pos, vel, acc, gps_manager->userdata);

	location_position_free(pos);
	location_velocity_free(vel);
	location_accuracy_free(acc);
	g_variant_unref(accuracy);
}

static void batch_callback(GVariant *param, void *user_data)
{
	MOD_LOGD("batch_callback");
	GpsManagerData *gps_manager = (GpsManagerData *)user_data;
	g_return_if_fail(gps_manager);
	g_return_if_fail(gps_manager->batch_cb);

	int num_of_location = 0;
	g_variant_get(param, "(i)", &num_of_location);

	gps_manager->batch_cb(TRUE, num_of_location, gps_manager->userdata);
}

static void on_signal_batch_callback(const gchar *sig, GVariant *param, gpointer user_data)
{
	if (!g_strcmp0(sig, "BatchChanged")) {
		MOD_LOGD("BatchChanged");
		batch_callback(param, user_data);
	} else {
		MOD_LOGD("Invaild signal[%s]", sig);
	}
}

static void on_signal_callback(const gchar *sig, GVariant *param, gpointer user_data)
{
	if (!g_strcmp0(sig, "SatelliteChanged")) {
		satellite_callback(param, user_data);
	} else if (!g_strcmp0(sig, "PositionChanged")) {
		position_callback(param, user_data);
	} else if (!g_strcmp0(sig, "StatusChanged")) {
		status_callback(param, user_data);
	} else {
		MOD_LOGD("Invaild signal[%s]", sig);
	}
}

static int start_batch(gpointer handle, LocModBatchExtCB batch_cb, guint batch_interval, guint batch_period, gpointer userdata)
{
	MOD_LOGD("start_batch");
	GpsManagerData *gps_manager = (GpsManagerData *) handle;
	g_return_val_if_fail(gps_manager, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(batch_cb, LOCATION_ERROR_NOT_AVAILABLE);

	gps_manager->batch_cb = batch_cb;
	gps_manager->userdata = userdata;

	int ret = LBS_CLIENT_ERROR_NONE;

	ret = lbs_client_create(LBS_CLIENT_METHOD_GPS , &(gps_manager->lbs_client));
	if (ret != LBS_CLIENT_ERROR_NONE || !gps_manager->lbs_client) {
		MOD_LOGE("Fail to create lbs_client_h. Error[%d]", ret);
		return LOCATION_ERROR_NOT_AVAILABLE;
	}
	MOD_LOGD("gps-manger(%x)", gps_manager);
	MOD_LOGD("batch (%x), user_data(%x)", gps_manager->batch_cb, gps_manager->userdata);

	ret = lbs_client_start_batch(gps_manager->lbs_client, LBS_CLIENT_BATCH_CB, on_signal_batch_callback, batch_interval, batch_period, gps_manager);
	if (ret != LBS_CLIENT_ERROR_NONE) {
		if (ret == LBS_CLIENT_ERROR_ACCESS_DENIED) {
			MOD_LOGE("Access denied[%d]", ret);
			return LOCATION_ERROR_NOT_ALLOWED;
		}
		MOD_LOGE("Fail to start lbs_client_h. Error[%d]", ret);
		lbs_client_destroy(gps_manager->lbs_client);
		gps_manager->lbs_client = NULL;

		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	return LOCATION_ERROR_NONE;
}

static int start(gpointer handle, guint pos_update_interval, LocModStatusCB status_cb, LocModPositionExtCB pos_cb, LocModSatelliteCB sat_cb, gpointer userdata)
{
	MOD_LOGD("start");
	GpsManagerData *gps_manager = (GpsManagerData *) handle;
	g_return_val_if_fail(gps_manager, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(status_cb, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(pos_cb, LOCATION_ERROR_NOT_AVAILABLE);

	gps_manager->status_cb = status_cb;
	gps_manager->pos_cb = pos_cb;
	gps_manager->sat_cb = sat_cb;
	gps_manager->userdata = userdata;

	int ret = LBS_CLIENT_ERROR_NONE;
	ret = lbs_client_create(LBS_CLIENT_METHOD_GPS , &(gps_manager->lbs_client));
	if (ret != LBS_CLIENT_ERROR_NONE || !gps_manager->lbs_client) {
		MOD_LOGE("Fail to create lbs_client_h. Error[%d]", ret);
		return LOCATION_ERROR_NOT_AVAILABLE;
	}
	MOD_LOGD("gps-manger(%x)", gps_manager);
	MOD_LOGD("pos_cb (%x), user_data(%x)", gps_manager->pos_cb, gps_manager->userdata);

	ret = lbs_client_start(gps_manager->lbs_client, pos_update_interval, LBS_CLIENT_LOCATION_CB | LBS_CLIENT_LOCATION_STATUS_CB | LBS_CLIENT_SATELLITE_CB | LBS_CLIENT_NMEA_CB, on_signal_callback, gps_manager);
	if (ret != LBS_CLIENT_ERROR_NONE) {
		if (ret == LBS_CLIENT_ERROR_ACCESS_DENIED) {
			MOD_LOGE("Access denied[%d]", ret);
			return LOCATION_ERROR_NOT_ALLOWED;
		}
		MOD_LOGE("Fail to start lbs_client_h. Error[%d]", ret);
		lbs_client_destroy(gps_manager->lbs_client);
		gps_manager->lbs_client = NULL;

		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	return LOCATION_ERROR_NONE;
}

static int stop_batch(gpointer handle)
{
	MOD_LOGD("stop_batch");
	GpsManagerData *gps_manager = (GpsManagerData *) handle;
	g_return_val_if_fail(gps_manager, LOCATION_ERROR_NOT_AVAILABLE);

	int ret = LBS_CLIENT_ERROR_NONE;

	ret = lbs_client_stop_batch(gps_manager->lbs_client);
	if (ret != LBS_CLIENT_ERROR_NONE) {
		MOD_LOGE("Fail to stop batch. Error[%d]", ret);
		lbs_client_destroy(gps_manager->lbs_client);
		gps_manager->lbs_client = NULL;
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	ret = lbs_client_destroy(gps_manager->lbs_client);
	if (ret != LBS_CLIENT_ERROR_NONE) {
		MOD_LOGE("Fail to destroy. Error[%d]", ret);
		return LOCATION_ERROR_NOT_AVAILABLE;
	}
	gps_manager->lbs_client = NULL;
	gps_manager->batch_cb = NULL;

	return LOCATION_ERROR_NONE;
}

static int stop(gpointer handle)
{
	MOD_LOGD("stop");
	GpsManagerData *gps_manager = (GpsManagerData *) handle;
	g_return_val_if_fail(gps_manager, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(gps_manager->lbs_client, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(gps_manager->status_cb, LOCATION_ERROR_NOT_AVAILABLE);

	int ret = LBS_CLIENT_ERROR_NONE;

	ret = lbs_client_stop(gps_manager->lbs_client);
	if (ret != LBS_CLIENT_ERROR_NONE) {
		MOD_LOGE("Fail to stop. Error[%d]", ret);
		lbs_client_destroy(gps_manager->lbs_client);
		gps_manager->lbs_client = NULL;
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	ret = lbs_client_destroy(gps_manager->lbs_client);
	if (ret != LBS_CLIENT_ERROR_NONE) {
		MOD_LOGE("Fail to destroy. Error[%d]", ret);
		return LOCATION_ERROR_NOT_AVAILABLE;
	}
	gps_manager->lbs_client = NULL;

	if (gps_manager->status_cb) {
		gps_manager->status_cb(FALSE, LOCATION_STATUS_NO_FIX, gps_manager->userdata);
	}

	gps_manager->status_cb = NULL;
	gps_manager->pos_cb = NULL;
	gps_manager->sat_cb = NULL;

	return LOCATION_ERROR_NONE;
}

static int get_nmea(gpointer handle, gchar **nmea_data)
{
	MOD_LOGD("get_nmea");
	GpsManagerData *gps_manager = (GpsManagerData *) handle;
	g_return_val_if_fail(gps_manager, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(nmea_data, LOCATION_ERROR_PARAMETER);

	gboolean ret = FALSE;
	int timestamp = 0;

	ret = lbs_client_get_nmea(gps_manager->lbs_client, &timestamp, nmea_data);
	if (ret) {
		MOD_LOGE("Error getting nmea: %d", ret);
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	return LOCATION_ERROR_NONE;
}

static int get_last_position(gpointer handle, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy)
{
	MOD_LOGD("get_last_position");
	GpsManagerData *gps_manager = (GpsManagerData *) handle;
	g_return_val_if_fail(gps_manager, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(position, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(velocity, LOCATION_ERROR_PARAMETER);
	g_return_val_if_fail(accuracy, LOCATION_ERROR_PARAMETER);

	int timestamp = 0;
	char location[128] = {0,};
	char *last_location[MAX_GPS_LOC_ITEM] = {0,};
	char *last = NULL;
	char *str = NULL;
	double longitude = 0.0, latitude = 0.0, altitude = 0.0;
	double speed = 0.0, direction = 0.0;
	double hor_accuracy = 0.0, ver_accuracy = 0.0;
	int index = 0;
	LocationStatus status = LOCATION_STATUS_NO_FIX;
	LocationAccuracyLevel level = LOCATION_ACCURACY_LEVEL_NONE;

	*position = NULL;
	*velocity = NULL;
	*accuracy = NULL;

	if (vconf_get_int(VCONFKEY_LOCATION_LAST_GPS_TIMESTAMP, &timestamp)) {
		MOD_LOGD("Error to get VCONFKEY_LOCATION_LAST_GPS_TIMESTAMP");
		return LOCATION_ERROR_NOT_AVAILABLE;
	} else {
		if (timestamp != 0) {
			if (vconf_get_dbl(VCONFKEY_LOCATION_LAST_GPS_LATITUDE, &latitude) ||
				vconf_get_dbl(VCONFKEY_LOCATION_LAST_GPS_LONGITUDE, &longitude) ||
				vconf_get_dbl(VCONFKEY_LOCATION_LAST_GPS_ALTITUDE, &altitude) ||
				vconf_get_dbl(VCONFKEY_LOCATION_LAST_GPS_SPEED, &speed) ||
				vconf_get_dbl(VCONFKEY_LOCATION_LAST_GPS_DIRECTION, &direction) ||
				vconf_get_dbl(VCONFKEY_LOCATION_LAST_GPS_HOR_ACCURACY, &hor_accuracy) ||
				vconf_get_dbl(VCONFKEY_LOCATION_LAST_GPS_VER_ACCURACY, &ver_accuracy)) {
				return LOCATION_ERROR_NOT_AVAILABLE;
			}
		} else {
			if (vconf_get_int(VCONFKEY_LOCATION_NV_LAST_GPS_TIMESTAMP, &timestamp)) {
				return LOCATION_ERROR_NOT_AVAILABLE;
			}
			str = vconf_get_str(VCONFKEY_LOCATION_NV_LAST_GPS_LOCATION);
			if (str == NULL) {
				return LOCATION_ERROR_NOT_AVAILABLE;
			}
			snprintf(location, sizeof(location), "%s", str);
			free(str);

			last_location[index] = (char *)strtok_r(location, ";", &last);
			while (last_location[index++] != NULL) {
				if (index == MAX_GPS_LOC_ITEM)
					break;
				last_location[index] = (char *)strtok_r(NULL, ";", &last);
			}
			if (index != MAX_GPS_LOC_ITEM) {
				return LOCATION_ERROR_NOT_AVAILABLE;
			}
			index = 0;

			latitude = strtod(last_location[index++], NULL);
			longitude = strtod(last_location[index++], NULL);
			altitude = strtod(last_location[index++], NULL);
			speed = strtod(last_location[index++], NULL);
			direction = strtod(last_location[index++], NULL);
			hor_accuracy = strtod(last_location[index++], NULL);
			ver_accuracy = strtod(last_location[index], NULL);
		}
	}

	if (timestamp) {
		status = LOCATION_STATUS_3D_FIX;
	} else {
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	level = LOCATION_ACCURACY_LEVEL_DETAILED;
	*position = location_position_new(timestamp, latitude, longitude, altitude, status);
	*velocity = location_velocity_new((guint) timestamp, speed, direction, 0.0);
	*accuracy = location_accuracy_new(level, hor_accuracy, ver_accuracy);

	return LOCATION_ERROR_NONE;
}

static int set_option(gpointer handle, const char *option)
{
	GpsManagerData *gps_manager = (GpsManagerData *) handle;
	g_return_val_if_fail(gps_manager, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(option, LOCATION_ERROR_PARAMETER);
	MOD_LOGD("set_option : %s", option);

	int ret = LBS_CLIENT_ERROR_NONE;
	ret = lbs_set_option(option);
	if (ret != LBS_AGPS_ERROR_NONE) {
		MOD_LOGE("Fail to lbs_set_option [%d]", ret);
		return LOCATION_ERROR_NOT_AVAILABLE;
	}

	return LOCATION_ERROR_NONE;
}

static int set_position_update_interval(gpointer handle, guint interval)
{
	MOD_LOGD("set_position_update_interval [%d]", interval);
	GpsManagerData *gps_manager = (GpsManagerData *) handle;
	g_return_val_if_fail(gps_manager, LOCATION_ERROR_NOT_AVAILABLE);
	g_return_val_if_fail(gps_manager->lbs_client, LOCATION_ERROR_NOT_AVAILABLE);

	int ret;
	ret = lbs_client_set_position_update_interval(gps_manager->lbs_client, interval);
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

LOCATION_MODULE_API gpointer init(LocModGpsOps *ops)
{
	MOD_LOGD("init");

	g_return_val_if_fail(ops, NULL);
	ops->start = start;
	ops->start_batch = start_batch;
	ops->stop = stop;
	ops->stop_batch = stop_batch;
	ops->get_last_position = get_last_position;
	ops->set_option = set_option;
	ops->set_position_update_interval = set_position_update_interval;

	Dl_info info;
	if (dladdr(&get_last_position, &info) == 0) {
		MOD_LOGE("Failed to get module name");
	} else if (g_strrstr(info.dli_fname, "gps")) {
		ops->get_nmea = get_nmea;
	} else {
		MOD_LOGE("Invalid module name");
	}

	GpsManagerData *gps_manager = g_new0(GpsManagerData, 1);
	g_return_val_if_fail(gps_manager, NULL);

	gps_manager->status_cb = NULL;
	gps_manager->pos_cb = NULL;
	gps_manager->batch_cb = NULL;
	gps_manager->userdata = NULL;
	gps_manager->is_started = FALSE;

	return (gpointer) gps_manager;
}

LOCATION_MODULE_API void shutdown(gpointer handle)
{
	MOD_LOGD("shutdown");
	g_return_if_fail(handle);
	GpsManagerData *gps_manager = (GpsManagerData *) handle;
	if (gps_manager->lbs_client) {
		lbs_client_stop(gps_manager->lbs_client);
		lbs_client_destroy(gps_manager->lbs_client);
		gps_manager->lbs_client = NULL;
	}

	gps_manager->status_cb = NULL;
	gps_manager->pos_cb = NULL;
	gps_manager->batch_cb = NULL;
	gps_manager->sat_cb = NULL;

	g_free(gps_manager);
	gps_manager = NULL;

}
