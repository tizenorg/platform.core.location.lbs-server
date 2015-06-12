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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "gps_plugin_data_types.h"
#include "last_position.h"
#include "debug_util.h"
#include "setting.h"

#define MAX_GPS_LOC_ITEM	7
#define UPDATE_INTERVAL		60

void gps_set_last_position(const pos_data_t *pos)
{
	if (pos == NULL) return;

	LOG_GPS(DBG_LOW, "set_last_position[%d]", pos->timestamp);

	char location[128] = {0,};
	snprintf(location, sizeof(location), "%.6lf;%.6lf;%.2lf;%.2lf;%.2lf;%.2lf;%.2lf;", pos->latitude, pos->longitude, pos->altitude, pos->speed, pos->bearing, pos->hor_accuracy, pos->ver_accuracy);

	setting_set_int(VCONFKEY_LOCATION_NV_LAST_GPS_TIMESTAMP, pos->timestamp);
	setting_set_string(VCONFKEY_LOCATION_NV_LAST_GPS_LOCATION, location);
}

void gps_set_position(const pos_data_t *pos)
{
	if (pos == NULL)
		return;

	LOG_GPS(DBG_LOW, "set_position[%d]", pos->timestamp);
	int last_timestamp = 0;
	int timestamp = pos->timestamp;
	double lat = pos->latitude;
	double lon = pos->longitude;
	double alt = pos->altitude;
	double spd = pos->speed;
	double dir = pos->bearing;
	double h_acc = pos->hor_accuracy;
	double v_acc = pos->ver_accuracy;

	setting_set_int(VCONFKEY_LOCATION_LAST_GPS_TIMESTAMP, timestamp);
	setting_set_double(VCONFKEY_LOCATION_LAST_GPS_LATITUDE, lat);
	setting_set_double(VCONFKEY_LOCATION_LAST_GPS_LONGITUDE, lon);
	setting_set_double(VCONFKEY_LOCATION_LAST_GPS_ALTITUDE, alt);
	setting_set_double(VCONFKEY_LOCATION_LAST_GPS_SPEED, spd);
	setting_set_double(VCONFKEY_LOCATION_LAST_GPS_DIRECTION, dir);
	setting_set_double(VCONFKEY_LOCATION_LAST_GPS_HOR_ACCURACY, h_acc);
	setting_set_double(VCONFKEY_LOCATION_LAST_GPS_VER_ACCURACY, v_acc);

	setting_get_int(VCONFKEY_LOCATION_NV_LAST_GPS_TIMESTAMP, &last_timestamp);

	if ((timestamp - last_timestamp) > UPDATE_INTERVAL) {
		gps_set_last_position(pos);
	}
}

void gps_get_last_position(pos_data_t *last_pos)
{
	if (last_pos == NULL)
		return;

	int timestamp;
	char location[128] = {0,};
	char *last_location[MAX_GPS_LOC_ITEM] = {0,};
	char *last = NULL;
	char *str = NULL;
	int index = 0;

	setting_get_int(VCONFKEY_LOCATION_LAST_GPS_TIMESTAMP, &timestamp);
	str = setting_get_string(VCONFKEY_LOCATION_NV_LAST_GPS_LOCATION);
	if (str == NULL) {
		return;
	}
	snprintf(location, sizeof(location), "%s", str);
	free(str);

	last_location[index] = (char *)strtok_r(location, ";", &last);
	while (last_location[index++] != NULL) {
		if (index == MAX_GPS_LOC_ITEM)
			break;
		last_location[index] = (char *)strtok_r(NULL, ";", &last);
	}
	index = 0;

	last_pos->timestamp = timestamp;
	last_pos->latitude = strtod(last_location[index++], NULL);
	last_pos->longitude = strtod(last_location[index++], NULL);
	last_pos->altitude = strtod(last_location[index++], NULL);
	last_pos->speed = strtod(last_location[index++], NULL);
	last_pos->bearing = strtod(last_location[index++], NULL);
	last_pos->hor_accuracy = strtod(last_location[index++], NULL);
	last_pos->ver_accuracy = strtod(last_location[index], NULL);

	LOG_GPS(DBG_LOW, "get_last_position[%d]", last_pos->timestamp);
}
