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

#ifndef _GPS_PLUGIN_EXTRA_DATA_TYPES_H_
#define _GPS_PLUGIN_EXTRA_DATA_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This enumeration has GPS session type.
 */
typedef enum {
	GPS_SESSION_SINGLE_FIX = 0,	/**< Single fix starting */
	GPS_SESSION_TRACKING_MODE	/**< Tracking mode starting */
} gps_session_t;

/**
 * This enumeration has GPS operation mode.
 */
typedef enum {
	GPS_OPERATION_STANDALONE = 0,	/**< GPS standalone (no assistance) */
	GPS_OPERATION_MS_BASED,			/**< MS-Based AGPS */
	GPS_OPERATION_MS_ASSISTED		/**< MS-Assisted AGPS */
} gps_operation_t;

/**
 * This enumeration has GPS starting type.
 */
typedef enum {
	GPS_STARTING_HOT_ = 0,	/**< Hot start */
	GPS_STARTING_COLD,		/**< Cold start */
	GPS_STARTING_NONE		/**< None */
} gps_starting_t;

/**
 * This enumeration has the SSL mode.
 */
typedef enum {
	AGPS_SSL_DISABLE = 0,	/**< SSL disable */
	AGPS_SSL_ENABLE			/**< SSL enable */
} agps_ssl_mode_t;

/**
 * This enumeration has the SSL certification type.
 */
typedef enum {
	AGPS_CERT_VERISIGN = 0,
	AGPS_CERT_THAWTE,
	AGPS_CERT_CMCC,
	AGPS_CERT_SPIRENT_TEST,
	AGPS_CERT_THALES_TEST,
	AGPS_CERT_CMCC_TEST,
	AGPS_CERT_BMC_TEST,
	AGPS_CERT_GOOGLE
} agps_ssl_cert_type_t;

/**
 * This enumeration has the verification confirm type.
 */
typedef enum {
	AGPS_VER_CNF_YES = 0x00,		/**< Specifies Confirmation yes. */
	AGPS_VER_CNF_NO = 0x01,			/**< Specifies Confirmation no. */
	AGPS_VER_CNF_NORESPONSE = 0x02	/**< Specifies Confirmation no response. */
} agps_verification_cnf_type_t;

/**
 * This enumeration has the zone in/out type.
 */
typedef enum {
	GEOFENCE_ZONE_OUT = 0x00,
	GEOFENCE_ZONE_IN = 0x01,
	GEOFENCE_ZONE_UNCERTAIN = 0x02
} geofence_zone_state_t;

/**
 * This structure is used to get the Extra Fix request parameters.
 */
typedef struct {
	int accuracy;			/**< accuracy */
	int tbf;				/**< time between fixes */
	int num_fixes;			/**< num fixes */
	unsigned char timeout;	/**< session timeout */
} gps_qos_param_t;

#ifdef __cplusplus
}
#endif

#endif /* _GPS_PLUGIN_EXTRA_DATA_TYPES_H_ */
