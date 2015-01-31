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

#ifndef _LBS_SERVER_H_
#define _LBS_SERVER_H_

#include <gps_plugin_data_types.h>
#include <gps_plugin_extra_data_types.h>

#define SERVICE_NAME	"org.tizen.lbs.Providers.LbsServer"
#define SERVICE_PATH	"/org/tizen/lbs/Providers/LbsServer"

#define MAX_NPS_LOC_ITEM	6
#define MAX_GPS_LOC_ITEM	7

#define UPDATE_INTERVAL		60

typedef enum {
	LBS_SERVER_METHOD_GPS = 0,
	LBS_SERVER_METHOD_NPS,
	LBS_SERVER_METHOD_AGPS,
	LBS_SERVER_METHOD_GEOFENCE,
	LBS_SERVER_METHOD_SIZE,
} lbs_server_method_e;

/**
 * This callback is called with position data.
 *
 * @param[in]	pos		Position data
 * @param[in]	error		Error report
 * @param[in]	user_data	User defined data
 */
typedef void (*gps_pos_cb) (pos_data_t * pos, gps_error_t error, void *user_data);

/**
 * This callback is called with batch data.
 *
 * @param[in]	batch		Batch data
 * @param[in]	user_data	User defined data
 */
typedef void (*gps_batch_cb) (batch_data_t * batch, void *user_data);

/**
 * This callback is called with satellite data.
 *
 * @param[in]	sv		Satellite data
 * @param[in]	user_data	User defined data
 */
typedef void (*gps_sv_cb) (sv_data_t * sv, void *user_data);

/**
 * This callback is called with nmea.
 *
 * @param[in]	nmea		NMEA data
 * @param[in]	user_data	User defined data
 */
typedef void (*gps_nmea_cb) (nmea_data_t * nmea, void *user_data);

/**
 * GPS callback structure.
 */
struct gps_callbacks {
	gps_pos_cb		pos_cb;		/**< Callback function for reporting position data */
	gps_batch_cb	batch_cb;	/**< Callback function for reporting batch data */
	gps_sv_cb		sv_cb;		/**< Callback function for reporting satellite data */
	gps_nmea_cb		nmea_cb;	/**< Callback function for reporting NMEA data */
};
typedef struct gps_callbacks gps_callbacks_t;

/**
 * LbsStatus
 *
 * defines the provider status
 **/
typedef enum {
	LBS_STATUS_ERROR,
	LBS_STATUS_UNAVAILABLE,
	LBS_STATUS_ACQUIRING,
	LBS_STATUS_AVAILABLE,
	LBS_STATUS_BATCH
} LbsStatus;

/**
 * LbsPositionExtFields:
 *
 * LbsPositionExtFields is a bitfield that defines the validity of
 * Position & Velocity values.
 **/
typedef enum {
	LBS_POSITION_EXT_FIELDS_NONE = 0,
	LBS_POSITION_EXT_FIELDS_LATITUDE = 1 << 0,
	LBS_POSITION_EXT_FIELDS_LONGITUDE = 1 << 1,
	LBS_POSITION_EXT_FIELDS_ALTITUDE = 1 << 2,
	LBS_POSITION_EXT_FIELDS_SPEED = 1 << 3,
	LBS_POSITION_EXT_FIELDS_DIRECTION = 1 << 4,
	LBS_POSITION_EXT_FIELDS_CLIMB = 1 << 5
} LbsPositionExtFields;

/**
 * LbsAccuracyLevel:
 *
 * Enum values used to define the approximate accuracy of
 * Position or Address information.
 **/
typedef enum {
	LBS_ACCURACY_LEVEL_NONE = 0,
	LBS_ACCURACY_LEVEL_COUNTRY,
	LBS_ACCURACY_LEVEL_REGION,
	LBS_ACCURACY_LEVEL_LOCALITY,
	LBS_ACCURACY_LEVEL_POSTALCODE,
	LBS_ACCURACY_LEVEL_STREET,
	LBS_ACCURACY_LEVEL_DETAILED,
} LbsAccuracyLevel;

typedef enum {
	GEOFENCE_ADD_FENCE = 0,
	GEOFENCE_DELETE_FENCE,
	GEOFENCE_PAUSE_FENCE,
	GEOFENCE_RESUME_FENCE
} geofence_event_e;

int gps_update_geofence_transition(int geofence_id, geofence_zone_state_t transition, double latitude, double longitude, double altitude, double speed, double bearing, double hor_accuracy, void *data);
int gps_update_geofence_service_status(int status, void *data);

// NPS

typedef enum {
	LBS_POSITION_FIELDS_NONE = 0,
	LBS_POSITION_FIELDS_LATITUDE = 1 << 0,
	LBS_POSITION_FIELDS_LONGITUDE = 1 << 1,
	LBS_POSITION_FIELDS_ALTITUDE = 1 << 2
} LbsPositionFields;	// not used

typedef struct {
	LbsPositionExtFields fields;
	int timestamp;
	double latitude;
	double longitude;
	double altitude;
	double speed;
	double direction;
	LbsAccuracyLevel acc_level;
	double hor_accuracy;
	double ver_accuracy;
} NpsManagerPositionExt;

typedef struct {
	int timestamp;
	double latitude;
	double longitude;
	double altitude;
	double speed;
	double direction;
	double hor_accuracy;
} NpsManagerLastPositionExt;

typedef void *NpsManagerHandle;

#endif
