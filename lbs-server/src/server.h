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

#ifndef _SERVER_H_
#define _SERVER_H_

#include "lbs_server.h"

#define SUPL_SERVER_URL_DEFAULT "your.supl-server.com"
#define SUPL_SERVER_PORT_DEFAULT 7275

/*This structure must be synchronized with State of mctxlDef.h */
typedef enum {
	GPS_STATE_AVAILABLE,
	GPS_STATE_OUT_OF_SERVICE,
	GPS_STATE_TEMPORARILY_UNAVAILABLE,
} gps_state_t;

typedef enum {
	GPS_SESSION_STOPPED,
	GPS_SESSION_STARTING,
	GPS_SESSION_STARTED,
	GPS_SESSION_STOPPING,
} gps_session_state_t;

typedef enum {
	AGPS_VERIFICATION_YES = 0x00,				/**<	Specifies Confirmation yes.	*/
	AGPS_VERIFICATION_NO = 0x01,				/**<	Specifies Confirmation no.	*/
	AGPS_VERIFICATION_NORESPONSE = 0x02,		/**<	Specifies Confirmation no response. */
} agps_verification_t;

typedef enum {
	GPS_MAIN_NOTIFY_NO_VERIFY = 0x00,
	GPS_MAIN_NOTIFY_ONLY = 0x01,
	GPS_MAIN_NOTIFY_ALLOW_NORESPONSE = 0x02,
	GPS_MAIN_NOTIFY_NOTALLOW_NORESPONSE = 0x03,
	GPS_MAIN_NOTIFY_PRIVACY_NEEDED = 0x04,
	GPS_MAIN_NOTIFY_PRIVACY_OVERRIDE = 0x05,
} gps_main_notify_type_t;

typedef struct {
	char requester_name[50];	/**< Specifies Requester name*/
	char client_name[50];		/**< Specifies Client name */
} gps_main_verif_info_t;

typedef struct {
	void *msg_body;
	int msg_size;
} gps_ni_data_t;

int initialize_server(int argc, char **argv);
int deinitialize_server();

int request_change_pos_update_interval_standalone_gps(unsigned int interval);
int request_start_batch_session(int batch_interval, int batch_period);
int request_stop_batch_session(void);
int request_start_session(int interval);
int request_stop_session(void);
#ifndef _TIZEN_PUBLIC_
int request_supl_ni_session(const char *header, const char *body, int size);
#endif
int register_update_callbacks(struct gps_callbacks *gps_callback, void *user_data);

int request_add_geofence(int fence_id, double latitude, double longitude, int radius, int last_state, int monitor_states, int notification_responsiveness, int unknown_timer);
int request_delete_geofence(int fence_id);
int request_pause_geofence(int fence_id);
int request_resume_geofence(int fence_id, int monitor_states);
int request_delete_gps_data(void);

int get_nmea_from_server(int *timestamp, char **nmea_data);

#endif				/* _SERVER_H_ */
