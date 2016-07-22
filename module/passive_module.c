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
#define MAX_NPS_LOC_ITEM	6

typedef struct {
	gpointer userdata;
} ModPassiveData;

static int get_last_position(gpointer handle, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy)
{
	MOD_LOGD("get_last_position");
	ModPassiveData *mod_passive = (ModPassiveData *) handle;
	g_return_val_if_fail(mod_passive, LOCATION_ERROR_NOT_AVAILABLE);
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
			if (vconf_get_int(VCONFKEY_LOCATION_NV_LAST_GPS_TIMESTAMP, &timestamp))
				return LOCATION_ERROR_NOT_AVAILABLE;

			str = vconf_get_str(VCONFKEY_LOCATION_NV_LAST_GPS_LOCATION);
			if (str == NULL)
				return LOCATION_ERROR_NOT_AVAILABLE;

			snprintf(location, sizeof(location), "%s", str);
			free(str);

			index = 0;
			last_location[index] = (char *)strtok_r(location, ";", &last);
			while (last_location[index] != NULL) {
				switch (index) {
				case 0:
					latitude = strtod(last_location[index], NULL);
					break;
				case 1:
					longitude = strtod(last_location[index], NULL);
					break;
				case 2:
					altitude = strtod(last_location[index], NULL);
					break;
				case 3:
					speed = strtod(last_location[index], NULL);
					break;
				case 4:
					direction = strtod(last_location[index], NULL);
					break;
				case 5:
					hor_accuracy = strtod(last_location[index], NULL);
					break;
				case 6:
					ver_accuracy = strtod(last_location[index], NULL);
					break;
				default:
					break;
				}
				if (++index == MAX_GPS_LOC_ITEM) break;
				last_location[index] = (char *)strtok_r(NULL, ";", &last);
			}
		}
	}

	if (timestamp)
		status = LOCATION_STATUS_3D_FIX;
	else
		return LOCATION_ERROR_NOT_AVAILABLE;

	level = LOCATION_ACCURACY_LEVEL_DETAILED;
	*position = location_position_new(timestamp, latitude, longitude, altitude, status);
	*velocity = location_velocity_new((guint) timestamp, speed, direction, 0.0);
	*accuracy = location_accuracy_new(level, hor_accuracy, ver_accuracy);

	return LOCATION_ERROR_NONE;
}

static int get_last_wps_position(gpointer handle, LocationPosition **position, LocationVelocity **velocity, LocationAccuracy **accuracy)
{
	MOD_LOGD("get_last_wps_position");
	ModPassiveData *mod_passive = (ModPassiveData *) handle;
	g_return_val_if_fail(mod_passive, LOCATION_ERROR_NOT_AVAILABLE);
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
		MOD_LOGD("Error to get VCONFKEY_LOCATION_LAST_WPS_TIMESTAMP");
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
				MOD_LOGD("Last timestamp failed");
				return LOCATION_ERROR_NOT_AVAILABLE;
			}
			str = vconf_get_str(VCONFKEY_LOCATION_NV_LAST_WPS_LOCATION);
			if (str == NULL) {
				MOD_LOGD("Last wps is null");
				return LOCATION_ERROR_NOT_AVAILABLE;
			}
			snprintf(location, sizeof(location), "%s", str);
			free(str);

			index = 0;
			last_location[index] = (char *)strtok_r(location, ";", &last);
			while (last_location[index] != NULL) {
				switch (index) {
				case 0:
					latitude = strtod(last_location[index], NULL);
					break;
				case 1:
					longitude = strtod(last_location[index], NULL);
					break;
				case 2:
					altitude = strtod(last_location[index], NULL);
					break;
				case 3:
					speed = strtod(last_location[index], NULL);
					break;
				case 4:
					direction = strtod(last_location[index], NULL);
					break;
				case 5:
					hor_accuracy = strtod(last_location[index], NULL);
					break;
				default:
					break;
				}
				if (++index == MAX_NPS_LOC_ITEM) break;
				last_location[index] = (char *)strtok_r(NULL, ";", &last);
			}
		}
	}

	if (timestamp)
		status = LOCATION_STATUS_3D_FIX;
	else
		return LOCATION_ERROR_NOT_AVAILABLE;

	level = LOCATION_ACCURACY_LEVEL_STREET;
	*position = location_position_new((guint) timestamp, latitude, longitude, altitude, status);
	*velocity = location_velocity_new((guint) timestamp, speed, direction, 0.0);
	*accuracy = location_accuracy_new(level, hor_accuracy, -1.0);

	return LOCATION_ERROR_NONE;
}

LOCATION_MODULE_API gpointer init(LocModPassiveOps *ops)
{
	MOD_LOGD("init");

	g_return_val_if_fail(ops, NULL);
	ops->get_last_position = get_last_position;
	ops->get_last_wps_position = get_last_wps_position;

	ModPassiveData *mod_passive = g_new0(ModPassiveData, 1);
	g_return_val_if_fail(mod_passive, NULL);

	mod_passive->userdata = NULL;

	return (gpointer) mod_passive;
}

LOCATION_MODULE_API void shutdown(gpointer handle)
{
	MOD_LOGD("shutdown");
	g_return_if_fail(handle);
	ModPassiveData *mod_passive = (ModPassiveData *) handle;

	g_free(mod_passive);
	mod_passive = NULL;

}
