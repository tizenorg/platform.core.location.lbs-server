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

#ifndef _GPS_PLUGIN_DATA_TYPES_H_
#define _GPS_PLUGIN_DATA_TYPES_H_

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif


/** Maximum number of satellite which is in used */
#define MAX_GPS_NUM_SAT_USED		(12)

/** Maximum number of satellite which is in view */
#define MAX_GPS_NUM_SAT_IN_VIEW		(32)

/**
 * This enumeration has error type.
 */
typedef enum {
	GPS_ERR_NONE			= 0,		/**< Error None */
	GPS_ERR_TIMEOUT			= -100,		/**< pos_cb error GPS Timeout */
	GPS_ERR_OUT_OF_SERVICE	= -101,		/**< pos_cb error GPS out of service */
	GPS_ERR_COMMUNICATION	= -200,		/**< Plugin event callback error */
} gps_error_t;

/**
 * This enumeration has geofence service status.
 */
typedef enum {
	GEOFENCE_STATUS_UNAVAILABLE	= 0,
	GEOFENCE_STATUS_AVAILABLE	= 1,
} geofence_status_t;

/**
 * This enumeration has geofence service error type.
 */
typedef enum {
	GEOFENCE_ERR_NONE				= 0,
	GEOFENCE_ERR_TOO_MANY_GEOFENCE	= -100,
	GEOFENCE_ERR_ID_EXISTS			= -101,
	GEOFENCE_ERR_ID_UNKNOWN			= -200,
	GEOFENCE_ERR_INVALID_TRANSITION	= -300,
	GEOFENCE_ERR_UNKNOWN			= -400,
} geofence_error_t;

/**
 * This structure defines the GPS position data.
 */
typedef struct {
	time_t	timestamp;		/**< Timestamp */
	double	latitude;		/**< Latitude data (in degree) */
	double	longitude;		/**< Longitude data (in degree) */
	double	altitude;		/**< Altitude data (in meter) */
	double	speed;			/**< Speed (in m/s) */
	double	bearing;		/**< Direction from true north(in degree) */
	double	hor_accuracy;	/**< Horizontal position error(in meter) */
	double	ver_accuracy;	/**< Vertical position error(in meter) */
} pos_data_t;

/**
 * This structure defines the GPS batch data.
 */
typedef struct {
	int	num_of_location;	/**< Number of batch data */
} batch_data_t;

/**
 * This structure defines the satellite data.
 */
typedef struct {
	int prn;			/**< Pseudo Random Noise code of satellite */
	int snr;			/**< Signal to Noise Ratio */
	int elevation;		/**< Elevation */
	int azimuth;		/**< Degrees from true north */
	int used;			/**< Satellite was used for position fix */
} sv_info_t;

/**
 * This structure defines the GPS satellite in view data.
 */
typedef struct {
	time_t timestamp;						/**< Timestamp */
	unsigned char pos_valid;				/**< TRUE, if position is valid */
	int num_of_sat;							/**< Number of satellites in view */
	sv_info_t sat[MAX_GPS_NUM_SAT_IN_VIEW];	/**< Satellite information */
} sv_data_t;

/**
 * This structure defines the NMEA data.
 */
typedef struct {
	time_t timestamp;	/**< Timestamp */
	int	len;			/**< NMEA data length */
	char *data;			/**< Raw NMEA data */
} nmea_data_t;

/**
 * This structure defines the geofence data.
 */
typedef struct {
	int	geofence_id;	/**< Geofence ID */
	double latitude;	/**< Latitude data (in degree) */
	double longitude;	/**< Longitude data (in degree) */
	int	radius;			/**< Radius data (in meters) */
} geofence_data_t;


#ifdef __cplusplus
}
#endif
#endif /* _GPS_PLUGIN_DATA_TYPES_H_ */
