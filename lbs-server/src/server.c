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
#include <signal.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

#include "server.h"
#include "gps_plugin_data_types.h"
#include "gps_plugin_intf.h"
#include "nmea_logger.h"
#include "data_connection.h"
#include "setting.h"
#include "gps_plugin_module.h"
#include "debug_util.h"
#include "last_position.h"
#include "dump_log.h"

#include "nps_plugin_module.h"
#include "nps_plugin_intf.h"

#ifdef _TIZEN_PUBLIC_
#include <msg.h>
#include <msg_transport.h>
#include <pmapi.h>
#endif

#include <vconf.h>
#include <vconf-keys.h>
#include <dlog.h>

#include <glib.h>
#include <glib-object.h>
#if !GLIB_CHECK_VERSION (2, 31, 0)
#include <glib/gthread.h>
#endif

#define REPLAY_MODULE	"replay"

#define PLATFORM_PATH	"/sys/devices/platform"
#define BRCM4752_PATH	PLATFORM_PATH"/bcm4752"
#define BRCM47520_PATH	PLATFORM_PATH"/bcm47520"
#define BRCM47511_PATH	PLATFORM_PATH"/bcm47511"
#define CSR_PATH		PLATFORM_PATH"/gsd4t"
#define QCOM8210_PATH	PLATFORM_PATH"/msm8210_gps"
#define QCOM8x30_PATH	PLATFORM_PATH"/msm8x30_gps"
#define QCOM9x15_PATH	PLATFORM_PATH"/msm9x15_gps"
#define QCOM8974_PATH	PLATFORM_PATH"/msm8974_gps"
#define QCOM8226_PATH	PLATFORM_PATH"/msm8226_gps"
#define QCOM8916_PATH	PLATFORM_PATH"/msm8916_gps"

#define MPS_TO_KMPH		3.6		/* 1 m/s = 3.6 km/h */

struct gps_callbacks g_update_cb;
void *g_user_data;

typedef struct {
	void *handle;
	char *name;
} gps_plugin_handler_t;

typedef struct {
	void *handle;
} nps_plugin_handler_t;

typedef struct {
	gps_session_state_t session_state;
	int dnet_used;
	gboolean logging_enabled;
	gboolean replay_enabled;

#ifdef _TIZEN_PUBLIC_
	pthread_t msg_thread;	/* Register SUPL NI cb to msg server Thread */
	pthread_t popup_thread;	/* Register SUPL NI cb to msg server Thread */
	int msg_thread_status;
#endif

	gps_plugin_handler_t gps_plugin;

	pos_data_t *pos_data;
	batch_data_t *batch_data;
	sv_data_t *sv_data;
	nmea_data_t *nmea_data;
} gps_server_t;

gps_server_t *g_gps_server = NULL;

nps_plugin_handler_t g_nps_plugin;

static void __nps_plugin_handler_deinit(void)
{
	if (g_nps_plugin.handle != NULL) {
		g_nps_plugin.handle = NULL;
	}
}

static int _gps_server_gps_event_cb(gps_event_info_t *gps_event_info, void *user_data);

static void _gps_nmea_changed_cb(keynode_t *key, void *data)
{
	gps_server_t *server = (gps_server_t *)data;
	int int_val;
	if (setting_get_int(VCONFKEY_LOCATION_NMEA_LOGGING, &int_val) == FALSE) {
		LOG_GPS(DBG_ERR, "//ERROR!! get VCONFKEY_LOCATION_NMEA_LOGGING setting");
		int_val = 0;
	}
	server->logging_enabled = (int_val == 1) ? TRUE : FALSE;
	return;
}

static gboolean get_replay_enabled()
{
	int int_val;
	gboolean ret;

	if (setting_get_int(VCONFKEY_LOCATION_REPLAY_ENABLED, &int_val) == FALSE) {
		LOG_GPS(DBG_ERR, "//ERROR get VCONFKEY_LOCATION_REPLAY_ENABLED setting");
		int_val = 0;
	}

	ret = (int_val == 1) ? TRUE : FALSE;

	return ret;
}

static void reload_plugin_module(gps_server_t *server)
{
	char *module_name;
	gps_failure_reason_t ReasonCode = GPS_FAILURE_CAUSE_NORMAL;

	if (server->replay_enabled == TRUE) {
		module_name = REPLAY_MODULE;
	} else {
		module_name = server->gps_plugin.name;
	}

	LOG_GPS(DBG_LOW, "Loading plugin.name : %s", module_name);

	if (!get_plugin_module()->deinit(&ReasonCode)) {
		LOG_GPS(DBG_ERR, "Fail to GPS plugin deinit");
	} else {
		unload_plugin_module(&server->gps_plugin.handle);
		if (!load_plugin_module(module_name, &server->gps_plugin.handle)) {
			LOG_GPS(DBG_ERR, "Fail to load %s plugin", module_name);
		} else {
			if (!get_plugin_module()->init(_gps_server_gps_event_cb, (void *)server)) {
				LOG_GPS(DBG_ERR, "Fail to %s plugin init", module_name);
				return;
			}
		}
	}
}

static void _gps_replay_changed_cb(keynode_t *key, void *data)
{
	gps_server_t *server = (gps_server_t *)data;
	server->replay_enabled = get_replay_enabled();
	reload_plugin_module(server);

	return;
}

static int _gps_server_get_gps_state()
{
	int val;

	if (setting_get_int(VCONFKEY_LOCATION_GPS_STATE, &val) == FALSE) {
		val = POSITION_OFF;
	}

	return val;
}

static void _gps_server_set_gps_state(int gps_state)
{
	int ret;

	switch (gps_state) {
		case POSITION_CONNECTED:
			ret = setting_set_int(VCONFKEY_LOCATION_GPS_STATE, POSITION_CONNECTED);
			gps_dump_log("GPS state : POSITION_CONNECTED", NULL);
			break;
		case POSITION_SEARCHING:
			ret = setting_set_int(VCONFKEY_LOCATION_GPS_STATE, POSITION_SEARCHING);
			gps_dump_log("GPS state : POSITION_SEARCHING", NULL);
			break;
		case POSITION_OFF:
			ret = setting_set_int(VCONFKEY_LOCATION_GPS_STATE, POSITION_OFF);
			gps_dump_log("GPS state : POSITION_OFF", NULL);
			break;
		default:
			ret = setting_set_int(VCONFKEY_LOCATION_GPS_STATE, POSITION_OFF);
			break;
	}

	if (ret == TRUE) {
		LOG_GPS(DBG_LOW, "Succesee to set VCONFKEY_LOCATION_GPS_STATE");
	} else {
		LOG_GPS(DBG_ERR, "Fail to set VCONF_LOCATION_GPS_STATE");
	}
}

int request_change_pos_update_interval_standalone_gps(unsigned int interval)
{
	LOG_GPS(DBG_INFO, "request_change_pos_update_interval_standalone_gps. interval[%u]", interval);
	gboolean status = TRUE;
	gps_failure_reason_t reason_code = GPS_FAILURE_CAUSE_NORMAL;
	gps_server_t *server = g_gps_server;

	if (server->session_state == GPS_SESSION_STARTING || server->session_state == GPS_SESSION_STARTED) {
		gps_action_change_interval_data_t gps_change_interval_data;
		gps_change_interval_data.interval = (int)interval;
		status = get_plugin_module()->request(GPS_ACTION_CHANGE_INTERVAL, &gps_change_interval_data, &reason_code);
		LOG_GPS(DBG_INFO, "requested go GPS module done. gps_change_interval_data.interval [%d]", gps_change_interval_data.interval);

		if (status == FALSE) {
			LOG_GPS(DBG_ERR, "Main: sending GPS_ACTION_CHANGE_INTERVAL Fail !");
			return FALSE;
		}
		LOG_GPS(DBG_INFO, "Main: sending GPS_ACTION_CHANGE_INTERVAL OK !");
		return TRUE;
	}
	return FALSE;
}

int request_start_session(int interval)
{
	LOG_GPS(DBG_INFO, "GPS start with interval [%d]", interval);

	gboolean status = TRUE;
	gps_failure_reason_t reason_code = GPS_FAILURE_CAUSE_NORMAL;
	gps_server_t *server = g_gps_server;
	gps_action_start_data_t gps_start_data;

	if (server->session_state != GPS_SESSION_STOPPED && server->session_state != GPS_SESSION_STOPPING) {
		LOG_GPS(DBG_WARN, "Main: GPS Session Already Started!");
		return TRUE;
	}

	server->session_state = GPS_SESSION_STARTING;
	LOG_GPS(DBG_LOW, "==GPSSessionState[%d]", server->session_state);
	gps_start_data.interval = interval;
	status = get_plugin_module()->request(GPS_ACTION_START_SESSION, &gps_start_data, &reason_code);

	if (status == FALSE) {
		LOG_GPS(DBG_ERR, "Main: sending GPS_ACTION_START_SESSION Fail !");
		return FALSE;
	}

	LOG_GPS(DBG_INFO, "Main: sending GPS_ACTION_START_SESSION OK !");

	setting_ignore_key_changed(VCONFKEY_LOCATION_REPLAY_ENABLED, _gps_replay_changed_cb);

	return TRUE;
}

int request_stop_session()
{
	gboolean status = TRUE;
	gboolean cur_replay_enabled = FALSE;
	gps_failure_reason_t reason_code = GPS_FAILURE_CAUSE_NORMAL;
	gps_server_t *server = g_gps_server;

	LOG_GPS(DBG_LOW, "Main: Stop GPS Session, ==GPSSessionState[%d]", server->session_state);
	if (server->session_state == GPS_SESSION_STARTED || server->session_state == GPS_SESSION_STARTING) {
		status = get_plugin_module()->request(GPS_ACTION_STOP_SESSION, NULL, &reason_code);
		if (status) {
			server->session_state = GPS_SESSION_STOPPING;
			LOG_GPS(DBG_LOW, "==GPSSessionState[%d]", server->session_state);
			cur_replay_enabled = get_replay_enabled();
			if (server->replay_enabled != cur_replay_enabled) {
				server->replay_enabled = cur_replay_enabled;
				reload_plugin_module(server);
			}
			setting_notify_key_changed(VCONFKEY_LOCATION_REPLAY_ENABLED, _gps_replay_changed_cb, (void *)server);
		} else {
			LOG_GPS(DBG_ERR, "plugin->request to LBS_GPS_STOP_SESSION Failed, reasonCode =%d", reason_code);
		}
	} else {
		/* If request is not sent, keep the client registed */
		LOG_GPS(DBG_LOW, "LBS_GPS_STOP_SESSION is not sent because the GPS state is not started, keep the client registed");
	}
	return status;
}

int request_start_batch_session(int batch_interval, int batch_period)
{
	LOG_GPS(DBG_INFO, "Batch: GPS start with interval[%d]", batch_interval);

	gboolean status = TRUE;
	gps_failure_reason_t reason_code = GPS_FAILURE_CAUSE_NORMAL;
	gps_server_t *server = g_gps_server;
	gps_action_start_data_t gps_start_data;

	if (server->session_state != GPS_SESSION_STOPPED && server->session_state != GPS_SESSION_STOPPING) {
		LOG_GPS(DBG_WARN, "Batch: GPS Session Already Started!");
		return TRUE;
	}

	server->session_state = GPS_SESSION_STARTING;
	LOG_GPS(DBG_LOW, "==GPSSessionState[%d]", server->session_state);
	gps_start_data.interval = batch_interval;
	gps_start_data.period = batch_period;
	status = get_plugin_module()->request(GPS_ACTION_START_BATCH, &gps_start_data, &reason_code);

	if (status == FALSE) {
		LOG_GPS(DBG_ERR, "Batch: sending GPS_ACTION_START_SESSION Fail !");
		return FALSE;
	}

	LOG_GPS(DBG_INFO, "Batch: sending GPS_ACTION_START_SESSION OK !");

	setting_ignore_key_changed(VCONFKEY_LOCATION_REPLAY_ENABLED, _gps_replay_changed_cb);

	return TRUE;
}

int request_stop_batch_session()
{
	gboolean status = TRUE;
	gboolean cur_replay_enabled = FALSE;
	gps_failure_reason_t reason_code = GPS_FAILURE_CAUSE_NORMAL;
	gps_server_t *server = g_gps_server;

	LOG_GPS(DBG_LOW, "Batch: Stop GPS Session, ==GPSSessionState[%d]", server->session_state);
	if (server->session_state == GPS_SESSION_STARTED || server->session_state == GPS_SESSION_STARTING) {
		status = get_plugin_module()->request(GPS_ACTION_STOP_BATCH, NULL, &reason_code);
		if (status) {
			server->session_state = GPS_SESSION_STOPPING;
			LOG_GPS(DBG_LOW, "==GPSSessionState[%d]", server->session_state);
			cur_replay_enabled = get_replay_enabled();
			if (server->replay_enabled != cur_replay_enabled) {
				server->replay_enabled = cur_replay_enabled;
				reload_plugin_module(server);
			}
			setting_notify_key_changed(VCONFKEY_LOCATION_REPLAY_ENABLED, _gps_replay_changed_cb, (void *)server);
		} else {
			LOG_GPS(DBG_ERR, "plugin->request to LBS_GPS_STOP_SESSION Failed, reasonCode =%d", reason_code);
		}
	} else {
		/* If request is not sent, keep the client registed */
		LOG_GPS(DBG_LOW, " LBS_GPS_STOP_SESSION is not sent because the GPS state is not started, keep the client registed ");
	}
	return status;
}

int request_add_geofence(int fence_id, double latitude, double longitude, int radius, int last_state, int monitor_states, int notification_responsiveness, int unknown_timer)
{
	gboolean status = FALSE;
	gps_failure_reason_t reason_code = GPS_FAILURE_CAUSE_NORMAL;
	geofence_action_data_t action_data;

	action_data.geofence.geofence_id = fence_id;
	action_data.geofence.latitude = latitude;
	action_data.geofence.longitude = longitude;
	action_data.geofence.radius = radius;
	action_data.last_state = last_state;
	action_data.monitor_states = monitor_states;
	/* Default value : temp */
	action_data.notification_responsiveness_ms = 5000;
	action_data.unknown_timer_ms = 30000;
	/*action_data.notification_responsiveness_ms = notification_responsiveness; */
	/*action_data.unknown_timer_ms = unknown_timer; */

	LOG_GPS(DBG_LOW, "request_add_geofence with geofence_id [%d]", fence_id);
	status = get_plugin_module()->request(GPS_ACTION_ADD_GEOFENCE, &action_data, &reason_code);

	return status;
}

int request_delete_geofence(int fence_id)
{
	gboolean status = FALSE;
	gps_failure_reason_t reason_code = GPS_FAILURE_CAUSE_NORMAL;

	LOG_GPS(DBG_LOW, "request_delete_geofence with geofence_id [%d]", fence_id);
	status = get_plugin_module()->request(GPS_ACTION_DELETE_GEOFENCE, &fence_id, &reason_code);

	return status;
}

int request_pause_geofence(int fence_id)
{
	gboolean status = FALSE;
	gps_failure_reason_t reason_code = GPS_FAILURE_CAUSE_NORMAL;

	LOG_GPS(DBG_LOW, "request_pause_geofence with geofence_id [%d]", fence_id);
	status = get_plugin_module()->request(GPS_ACTION_PAUSE_GEOFENCE, &fence_id, &reason_code);

	return status;
}

int request_resume_geofence(int fence_id, int monitor_states)
{
	gboolean status = FALSE;
	gps_failure_reason_t reason_code = GPS_FAILURE_CAUSE_NORMAL;
	geofence_action_data_t action_data;

	action_data.geofence.geofence_id = fence_id;
	action_data.monitor_states = monitor_states;

	LOG_GPS(DBG_LOW, "request_resume_geofence with geofence_id [%d]", fence_id);
	status = get_plugin_module()->request(GPS_ACTION_RESUME_GEOFENCE, &action_data, &reason_code);

	return status;
}

int request_delete_gps_data()
{
	gboolean status = TRUE;
	gps_failure_reason_t reason_code = GPS_FAILURE_CAUSE_NORMAL;

	status = get_plugin_module()->request(GPS_ACTION_DELETE_GPS_DATA, NULL, &reason_code);

	if (status == FALSE) {
		LOG_GPS(DBG_ERR, "Fail : GPS_ACTION_DELETE_GPS_DATA [%d]", reason_code);
		return FALSE;
	}

	LOG_GPS(DBG_LOW, "Success to GPS_ACTION_DELETE_GPS_DATA");
	return TRUE;
}

int get_nmea_from_server(int *timestamp, char **nmea_data)
{
	LOG_GPS(DBG_INFO, "ENTER >>>");

	gps_server_t *server = g_gps_server;

	if (server->session_state == GPS_SESSION_STARTING || server->session_state == GPS_SESSION_STARTED) {
		*timestamp = (int) server->nmea_data->timestamp;
		*nmea_data = g_strndup(server->nmea_data->data, server->nmea_data->len);
	}
	LOG_GPS(DBG_LOW, "=== timestmap: %d, nmea_data: %s", *timestamp, *nmea_data);
	return TRUE;
}

static void _gps_plugin_handler_init(gps_server_t *server, char *module_name)
{
	if (module_name == NULL) {
		LOG_GPS(DBG_ERR, "Fail : module_name is NULL");
		return;
	}

	server->gps_plugin.handle = NULL;
	server->gps_plugin.name = (char *)malloc(strlen(module_name) + 1);
	g_return_if_fail(server->gps_plugin.name);

	g_strlcpy(server->gps_plugin.name, module_name, strlen(module_name) + 1);
}

static void _gps_plugin_handler_deinit(gps_server_t *server)
{
	if (server->gps_plugin.handle != NULL) {
		server->gps_plugin.handle = NULL;
	}

	if (server->gps_plugin.name != NULL) {
		free(server->gps_plugin.name);
		server->gps_plugin.name = NULL;
	}
}

static void _gps_get_nmea_replay_mode(gps_server_t *server)
{
	int int_val = 0;

	if (setting_get_int(VCONFKEY_LOCATION_NMEA_LOGGING, &int_val) == FALSE) {
		LOG_GPS(DBG_ERR, "//ERROR!! get VCONFKEY_LOCATION_NMEA_LOGGING setting");
		int_val = 0;
	}
	server->logging_enabled = (int_val == 1) ? TRUE : FALSE;

	if (setting_get_int(VCONFKEY_LOCATION_REPLAY_ENABLED, &int_val) == FALSE) {
		LOG_GPS(DBG_ERR, "//ERROR!! get VCONFKEY_LOCATION_REPLAY_ENABLED setting");
		int_val = 0;
	}
	server->replay_enabled = (int_val == 1) ? TRUE : FALSE;
}

static void _gps_notify_params(gps_server_t *server)
{
	setting_notify_key_changed(VCONFKEY_LOCATION_NMEA_LOGGING, _gps_nmea_changed_cb, (void *)server);
	setting_notify_key_changed(VCONFKEY_LOCATION_REPLAY_ENABLED, _gps_replay_changed_cb, (void *)server);
}

static void _gps_ignore_params()
{
	setting_ignore_key_changed(VCONFKEY_LOCATION_NMEA_LOGGING, _gps_nmea_changed_cb);
	setting_ignore_key_changed(VCONFKEY_LOCATION_REPLAY_ENABLED, _gps_replay_changed_cb);
}

static void _gps_server_start_event(gps_server_t *server)
{
	LOG_GPS(DBG_LOW, "==GPSSessionState [%d] -> [%d]", server->session_state, GPS_SESSION_STARTED);
	server->session_state = GPS_SESSION_STARTED;

	if (server->logging_enabled) {
		LOG_GPS(DBG_LOW, "NMEA STARTED");
		start_nmea_log();
	}

	if (server->pos_data == NULL) {
		server->pos_data = (pos_data_t *) malloc(sizeof(pos_data_t));
		if (server->pos_data == NULL) {
			LOG_GPS(DBG_WARN, "//callback: server->pos_data re-malloc Failed!!");
		} else {
			memset(server->pos_data, 0x00, sizeof(pos_data_t));
		}
	}
	if (server->batch_data == NULL) {
		server->batch_data = (batch_data_t *) malloc(sizeof(batch_data_t));
		if (server->batch_data == NULL) {
			LOG_GPS(DBG_WARN, "//callback: server->batch_data re-malloc Failed!!");
		} else {
			memset(server->batch_data, 0x00, sizeof(batch_data_t));
		}
	}
	if (server->sv_data == NULL) {
		server->sv_data = (sv_data_t *) malloc(sizeof(sv_data_t));
		if (server->sv_data == NULL) {
			LOG_GPS(DBG_WARN, "//callback: server->sv_data re-malloc Failed!!");
		} else {
			memset(server->sv_data, 0x00, sizeof(sv_data_t));
		}
	}
	if (server->nmea_data == NULL) {
		server->nmea_data = (nmea_data_t *) malloc(sizeof(nmea_data_t));
		if (server->nmea_data == NULL) {
			LOG_GPS(DBG_WARN, "//callback: server->nmea_data re-malloc Failed!!");
		} else {
			memset(server->nmea_data, 0x00, sizeof(nmea_data_t));
		}
	}

	_gps_server_set_gps_state(POSITION_SEARCHING);
#ifdef _TIZEN_PUBLIC_
	pm_lock_state(LCD_OFF, STAY_CUR_STATE, 0);
#endif
}

static void _gps_server_stop_event(gps_server_t *server)
{
	LOG_GPS(DBG_LOW, "==GPSSessionState [%d] -> [%d]", server->session_state, GPS_SESSION_STOPPED);
	server->session_state = GPS_SESSION_STOPPED;

	_gps_server_set_gps_state(POSITION_OFF);
#ifdef _TIZEN_PUBLIC_
	pm_unlock_state(LCD_OFF, PM_RESET_TIMER);
#endif

	if (server->logging_enabled) {
		LOG_GPS(DBG_LOW, "NMEA STOPPED");
		stop_nmea_log();
	}
}

static void _report_pos_event(gps_server_t *server, gps_event_info_t *gps_event)
{
	if (server->pos_data != NULL) {
		memset(server->pos_data, 0x00, sizeof(pos_data_t)); /* Moved: the address of server->pos_data sometimes returns 0 when stopping GPS */
		memcpy(server->pos_data, &(gps_event->event_data.pos_ind.pos), sizeof(pos_data_t));
		/* change m/s to km/h */
		server->pos_data->speed = server->pos_data->speed * MPS_TO_KMPH;
		gps_set_position(server->pos_data);
		g_update_cb.pos_cb(server->pos_data, gps_event->event_data.pos_ind.error, g_user_data);
	} else {
		LOG_GPS(DBG_ERR, "server->pos_data is NULL");
	}
}

static void _report_batch_event(gps_server_t *server, gps_event_info_t *gps_event)
{
	if (server->batch_data != NULL) {
		memset(server->batch_data, 0x00, sizeof(batch_data_t));
		memcpy(server->batch_data, &(gps_event->event_data.batch_ind.batch), sizeof(batch_data_t));
		g_update_cb.batch_cb(server->batch_data, g_user_data);
	} else {
		LOG_GPS(DBG_ERR, "server->batch_data is NULL");
	}
}

static void _report_sv_event(gps_server_t *server, gps_event_info_t *gps_event)
{
	if (server->sv_data != NULL) {
		memset(server->sv_data, 0x00, sizeof(sv_data_t));
		memcpy(server->sv_data, &(gps_event->event_data.sv_ind.sv), sizeof(sv_data_t));
		g_update_cb.sv_cb(server->sv_data, g_user_data);
	} else {
		LOG_GPS(DBG_ERR, "server->sv_data is NULL");
	}
}

static void _report_nmea_event(gps_server_t *server, gps_event_info_t *gps_event)
{
	if (server->nmea_data == NULL) {
		LOG_GPS(DBG_ERR, "server->nmea_data is NULL");
		return;
	}

	if (server->nmea_data->data) {
		free(server->nmea_data->data);
		server->nmea_data->data = NULL;
	}

	/*LOG_GPS(DBG_LOW, "server->nmea_data->len : [%d]", gps_event->event_data.nmea_ind.nmea.len); */
	server->nmea_data->len = gps_event->event_data.nmea_ind.nmea.len;
	server->nmea_data->data = (char *)malloc(server->nmea_data->len);
	if (server->nmea_data->data == NULL) {
		LOG_GPS(DBG_ERR, "Fail to malloc of server->nmea_data->data");
		return;
	}
	memset(server->nmea_data->data, 0x00, server->nmea_data->len);
	memcpy(server->nmea_data->data, gps_event->event_data.nmea_ind.nmea.data, server->nmea_data->len);
	g_update_cb.nmea_cb(server->nmea_data, g_user_data);

	if (server->logging_enabled) {
		int nmea_len;
		char *p_nmea_data;
		nmea_len = gps_event->event_data.nmea_ind.nmea.len;
		p_nmea_data = gps_event->event_data.nmea_ind.nmea.data;
		write_nmea_log(p_nmea_data, nmea_len);
	}
}

static int _gps_server_open_data_connection(gps_server_t *server)
{
	LOG_GPS(DBG_LOW, "Enter _gps_server_open_data_connection");

	server->dnet_used++;

	if (server->dnet_used > 1) {
		LOG_GPS(DBG_LOW, "dnet_used : count[ %d ]", server->dnet_used);
		return TRUE;
	} else {
		LOG_GPS(DBG_LOW, "First open the data connection");
	}

	unsigned char result;

	result = start_pdp_connection();

	if (result == TRUE) {
		LOG_GPS(DBG_LOW, "//Open PDP Conn for SUPL Success.");
	} else {
		LOG_GPS(DBG_ERR, "//Open PDP Conn for SUPL Fail.");
		return FALSE;
	}
	return TRUE;
}

static int _gps_server_close_data_connection(gps_server_t *server)
{
	LOG_GPS(DBG_LOW, "Enter _gps_server_close_data_connection");

	if (server->dnet_used > 0) {
		server->dnet_used--;
	}

	if (server->dnet_used != 0) {
		LOG_GPS(DBG_LOW, "Cannot stop the data connection! [ dnet_used : %d ]", server->dnet_used);
		return TRUE;
	} else {
		LOG_GPS(DBG_LOW, "Close the data connection");
	}

	unsigned char result;

	result = stop_pdp_connection();
	if (result == TRUE) {
		LOG_GPS(DBG_LOW, "Close PDP Conn for SUPL Success.");
	} else {
		LOG_GPS(DBG_ERR, "//Close PDP Conn for SUPL Fail.");
		return FALSE;
	}

	return TRUE;
}

static int _gps_server_resolve_dns(char *domain)
{
	LOG_GPS(DBG_LOW, "Enter _gps_server_resolve_dns");

	unsigned char result;
	gps_failure_reason_t reason_code = GPS_FAILURE_CAUSE_NORMAL;
	unsigned int ipaddr;
	int port;

	result = query_dns(domain, &ipaddr, &port);
	if (result == TRUE) {
		LOG_GPS(DBG_LOW, "Success to resolve domain name [ %s ] / ipaddr [ %u ]", domain, ipaddr);
		get_plugin_module()->request(GPS_INDI_SUPL_DNSQUERY, (void *)(&ipaddr), &reason_code);
	} else {
		ipaddr = 0;
		LOG_GPS(DBG_ERR, "Fail to resolve domain name [ %s ] / ipaddr [ %u ]", domain, ipaddr);
		get_plugin_module()->request(GPS_INDI_SUPL_DNSQUERY, (void *)(&ipaddr), &reason_code);
		return FALSE;
	}

	return TRUE;
}

static void _gps_server_send_facttest_result(double snr_data, int prn, unsigned short result)
{
}

static void _report_geofence_transition(int geofence_id, geofence_zone_state_t transition, double latitude, double longitude, double altitude, double speed, double bearing, double hor_accuracy)
{
	gps_update_geofence_transition(geofence_id, transition, latitude, longitude, altitude, speed, bearing, hor_accuracy, (void *)g_user_data);
}

static void _report_geofence_service_status(int status)
{
	gps_update_geofence_service_status(status, (void *)g_user_data);
}

static void _gps_server_send_geofence_result(geofence_event_e event, int geofence_id, int result)
{
	if (result != 0) {
		LOG_GPS(DBG_ERR, "result is [%d]", result);
		return;
	}

	switch (event) {
		case GEOFENCE_ADD_FENCE:
			LOG_GPS(DBG_LOW, "Geofence ID[%d], Success ADD_GEOFENCE", geofence_id);
			break;
		case GEOFENCE_DELETE_FENCE:
			LOG_GPS(DBG_LOW, "Geofence ID[%d], Success DELETE_GEOFENCE", geofence_id);
			break;
		case GEOFENCE_PAUSE_FENCE:
			LOG_GPS(DBG_LOW, "Geofence ID[%d], Success PAUSE_GEOFENCE", geofence_id);
			break;
		case GEOFENCE_RESUME_FENCE:
			LOG_GPS(DBG_LOW, "Geofence ID[%d], Success RESUME_GEOFENCE", geofence_id);
			break;
		default:
			break;
	}
}

static int _gps_server_gps_event_cb(gps_event_info_t *gps_event_info, void *user_data)
{
	/*FUNC_ENTRANCE_SERVER; */
	gps_server_t *server = (gps_server_t *)user_data;
	int result = TRUE;
	if (gps_event_info == NULL) {
		LOG_GPS(DBG_WARN, "//NULL pointer variable passed");
		return result;
	}

	switch (gps_event_info->event_id) {
		case GPS_EVENT_START_SESSION:
			LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_START_SESSION :::::::::::::::");
			if (gps_event_info->event_data.start_session_rsp.error == GPS_ERR_NONE) {
				_gps_server_start_event(server);
			} else {
				LOG_GPS(DBG_ERR, "//Start Session Failed, error : %d",
						gps_event_info->event_data.start_session_rsp.error);
			}
			break;
		case GPS_EVENT_SET_OPTION: {
				LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_SET_OPTION :::::::::::::::");
				if (gps_event_info->event_data.set_option_rsp.error != GPS_ERR_NONE) {
					LOG_GPS(DBG_ERR, "//Set Option Failed, error : %d",
							gps_event_info->event_data.set_option_rsp.error);
				}
			}
			break;

		case GPS_EVENT_STOP_SESSION:
			LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_STOP_SESSION :::::::::::::::");
			if (gps_event_info->event_data.stop_session_rsp.error == GPS_ERR_NONE) {
				_gps_server_close_data_connection(server);
				_gps_server_stop_event(server);
			} else {
				LOG_GPS(DBG_ERR, "//Stop Session Failed, error : %d",
						gps_event_info->event_data.stop_session_rsp.error);
			}

			break;
		case GPS_EVENT_CHANGE_INTERVAL:
			LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_CHANGE_INTERVAL :::::::::::::::");
			if (gps_event_info->event_data.change_interval_rsp.error == GPS_ERR_NONE) {
				LOG_GPS(DBG_LOW, "Change interval success.");
			} else {
				LOG_GPS(DBG_ERR, "//Change interval Failed, error : %d",
						gps_event_info->event_data.change_interval_rsp.error);
			}
			break;
		case GPS_EVENT_REPORT_POSITION:
			LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_POSITION :::::::::::::::");
			if (gps_event_info->event_data.pos_ind.error == GPS_ERR_NONE) {
				_report_pos_event(server, gps_event_info);
			} else {
				LOG_GPS(DBG_ERR, "GPS_EVENT_POSITION Failed, error : %d", gps_event_info->event_data.pos_ind.error);
			}
			break;
		case GPS_EVENT_REPORT_BATCH:
			LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_BATCH :::::::::::::::");
			if (gps_event_info->event_data.batch_ind.error == GPS_ERR_NONE) {
				_report_batch_event(server, gps_event_info);
			} else {
				LOG_GPS(DBG_ERR, "GPS_EVENT_BATCH Failed, error : %d", gps_event_info->event_data.batch_ind.error);
			}
			break;
		case GPS_EVENT_REPORT_SATELLITE:
			if (gps_event_info->event_data.sv_ind.error == GPS_ERR_NONE) {
				if (gps_event_info->event_data.sv_ind.sv.pos_valid) {
					if (_gps_server_get_gps_state() != POSITION_CONNECTED)
						_gps_server_set_gps_state(POSITION_CONNECTED);
				} else {
					LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_SATELLITE :::::::::::::::");
					if (_gps_server_get_gps_state() != POSITION_SEARCHING)
						_gps_server_set_gps_state(POSITION_SEARCHING);
				}
				_report_sv_event(server, gps_event_info);
			} else {
				LOG_GPS(DBG_ERR, "GPS_EVENT_SATELLITE Failed, error : %d", gps_event_info->event_data.sv_ind.error);
			}
			break;
		case GPS_EVENT_REPORT_NMEA:
			if (_gps_server_get_gps_state() != POSITION_CONNECTED) {
				/*LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_NMEA :::::::::::::::"); */
			}

			if (gps_event_info->event_data.nmea_ind.error == GPS_ERR_NONE) {
				_report_nmea_event(server, gps_event_info);
			} else {
				LOG_GPS(DBG_ERR, "GPS_EVENT_NMEA Failed, error : %d", gps_event_info->event_data.nmea_ind.error);
			}
			break;
		case GPS_EVENT_ERR_CAUSE:
			break;
		case GPS_EVENT_AGPS_VERIFICATION_INDI:
			break;
		case GPS_EVENT_GET_IMSI:
			break;
		case GPS_EVENT_GET_REF_LOCATION:
			break;
		case GPS_EVENT_OPEN_DATA_CONNECTION: {
				LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_OPEN_DATA_CONNECTION :::::::::::::::");
				result = _gps_server_open_data_connection(server);
			}
			break;
		case GPS_EVENT_CLOSE_DATA_CONNECTION: {
				LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_CLOSE_DATA_CONNECTION :::::::::::::::");
				result = _gps_server_close_data_connection(server);
			}
			break;
		case GPS_EVENT_DNS_LOOKUP_IND:
			LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_DNS_LOOKUP_IND :::::::::::::::");
			if (gps_event_info->event_data.dns_query_ind.error == GPS_ERR_NONE) {
				result = _gps_server_resolve_dns(gps_event_info->event_data.dns_query_ind.domain_name);
			} else {
				result = FALSE;
			}
			if (result == TRUE) {
				LOG_GPS(DBG_LOW, "Success to get the DNS Query about [ %s ]",
						gps_event_info->event_data.dns_query_ind.domain_name);
			}
			break;
		case GPS_EVENT_FACTORY_TEST:
			LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_FACTORY_TEST :::::::::::::::");
			if (gps_event_info->event_data.factory_test_rsp.error == GPS_ERR_NONE) {
				LOG_GPS(DBG_LOW, "[LBS server] Response Factory test result success");
				_gps_server_send_facttest_result(gps_event_info->event_data.factory_test_rsp.snr,
												 gps_event_info->event_data.factory_test_rsp.prn, TRUE);
			} else {
				LOG_GPS(DBG_ERR, "//[LBS server] Response Factory test result ERROR");
				_gps_server_send_facttest_result(gps_event_info->event_data.factory_test_rsp.snr,
												 gps_event_info->event_data.factory_test_rsp.prn, FALSE);
			}
			break;
		case GPS_EVENT_GEOFENCE_TRANSITION:
			LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_GEOFENCE_TRANSITION :::::::::::::::");
			_report_geofence_transition(gps_event_info->event_data.geofence_transition_ind.geofence_id,
										gps_event_info->event_data.geofence_transition_ind.state,
										gps_event_info->event_data.geofence_transition_ind.pos.latitude,
										gps_event_info->event_data.geofence_transition_ind.pos.longitude,
										gps_event_info->event_data.geofence_transition_ind.pos.altitude,
										gps_event_info->event_data.geofence_transition_ind.pos.speed,
										gps_event_info->event_data.geofence_transition_ind.pos.bearing,
										gps_event_info->event_data.geofence_transition_ind.pos.hor_accuracy);
			break;
		case GPS_EVENT_GEOFENCE_STATUS:
			LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_GEOFENCE_STATUS :::::::::::::::");
			_report_geofence_service_status(gps_event_info->event_data.geofence_status_ind.status);
			break;
		case GPS_EVENT_ADD_GEOFENCE:
			LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_ADD_GEOFENCE :::::::::::::::");
			_gps_server_send_geofence_result(GEOFENCE_ADD_FENCE,
											 gps_event_info->event_data.geofence_event_rsp.geofence_id,
											 gps_event_info->event_data.geofence_event_rsp.error);
			break;
		case GPS_EVENT_DELETE_GEOFENCE:
			LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_DELETE_GEOFENCE :::::::::::::::");
			_gps_server_send_geofence_result(GEOFENCE_DELETE_FENCE,
											 gps_event_info->event_data.geofence_event_rsp.geofence_id,
											 gps_event_info->event_data.geofence_event_rsp.error);
			break;
		case GPS_EVENT_PAUSE_GEOFENCE:
			LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_PAUSE_GEOFENCE :::::::::::::::");
			_gps_server_send_geofence_result(GEOFENCE_PAUSE_FENCE,
											 gps_event_info->event_data.geofence_event_rsp.geofence_id,
											 gps_event_info->event_data.geofence_event_rsp.error);
			break;
		case GPS_EVENT_RESUME_GEOFENCE:
			LOG_GPS(DBG_LOW, "<<::::::::::: GPS_EVENT_RESUME_GEOFENCE :::::::::::::::");
			_gps_server_send_geofence_result(GEOFENCE_RESUME_FENCE,
											 gps_event_info->event_data.geofence_event_rsp.geofence_id,
											 gps_event_info->event_data.geofence_event_rsp.error);
			break;
		default:
			LOG_GPS(DBG_WARN, "//Error: Isettingalid Event Type %d", gps_event_info->event_id);
			break;
	}
	return result;
}

#ifndef _TIZEN_PUBLIC_
int request_supl_ni_session(const char *header, const char *body, int size)
{
	agps_supl_ni_info_t info;
	gps_failure_reason_t reason_code;

	info.msg_body = (char *)body;
	info.msg_size = size;
	info.status = TRUE;

	if (!get_plugin_module()->request(GPS_ACTION_REQUEST_SUPL_NI, &info, &reason_code)) {
		LOG_GPS(DBG_ERR, "Failed to request SUPL NI (code:%d)", reason_code);
		return FALSE;
	}

	return TRUE;
}
#else
static void *request_supl_ni_session(void *data)
{
	gps_ni_data_t *ni_data = (gps_ni_data_t *) data;
	agps_supl_ni_info_t info;
	gps_failure_reason_t reason_code;

	info.msg_body = (char *)ni_data->msg_body;
	info.msg_size = ni_data->msg_size;
	info.status = TRUE;

	if (!get_plugin_module()->request(GPS_ACTION_REQUEST_SUPL_NI, &info, &reason_code)) {
		LOG_GPS(DBG_ERR, "Failed to request SUPL NI (code:%d)", reason_code);
	}

	free(ni_data);

	return NULL;
}

static void _gps_supl_networkinit_smscb(msg_handle_t hMsgHandle, msg_struct_t msg, void *user_param)
{
	LOG_GPS(DBG_ERR, "_gps_supl_networkinit_smscb is called");
	gps_server_t *server = (gps_server_t *)user_param;

	gps_ni_data_t *new_message = NULL;

	new_message = (gps_ni_data_t *)malloc(sizeof(gps_ni_data_t));
	memset(new_message, 0x00, sizeof(gps_ni_data_t));
	msg_get_int_value(msg, MSG_MESSAGE_DATA_SIZE_INT, &new_message->msg_size);
	msg_get_str_value(msg, MSG_MESSAGE_SMS_DATA_STR, new_message->msg_body, new_message->msg_size);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&server->popup_thread, &attr, request_supl_ni_session, new_message) != 0) {
		LOG_GPS(DBG_WARN, "Can not make pthread......");
	}
}

static void _gps_supl_networkinit_wappushcb(msg_handle_t hMsgHandle, const char *pPushHeader, const char *pPushBody,
		int pushBodyLen, void *user_param)
{
	gps_server_t *server = (gps_server_t *)user_param;
	char *str;

	LOG_GPS(DBG_ERR, "_gps_supl_networkinit_wappushcb is called");

	if (strstr(pPushHeader, "application/vnd.omaloc-supl-init") != NULL) {
		str = strstr(pPushHeader, "X-Wap-Application-Id:");

		if (strncmp(str + 22, "x-oma-application:ulp.ua", 24) != 0) {
			LOG_GPS(DBG_ERR, "Wrong Application-ID");
			return;
		}
	}

	gps_ni_data_t *new_message = NULL;

	new_message = (gps_ni_data_t *) malloc(sizeof(gps_ni_data_t));
	new_message->msg_body = (char *)pPushBody;
	new_message->msg_size = pushBodyLen;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&server->popup_thread, &attr, request_supl_ni_session, new_message) != 0) {
		LOG_GPS(DBG_WARN, "Can not make pthread......");
	}
}

static void *_gps_register_msgfwcb(void *data)
{
	gps_server_t *server = (gps_server_t *)data;
	msg_handle_t msgHandle = NULL;
	msg_error_t err = MSG_SUCCESS;

	int setValue = 0;
	int ret;

	int cnt = 60;

	while (cnt > 0) {
		ret = vconf_get_bool(VCONFKEY_MSG_SERVER_READY, &setValue);

		if (ret == -1) {
			LOG_GPS(DBG_WARN, "Fail to get VCONFKEY_MSG_SERVER_READY");
			return NULL;
		}

		if (setValue) {
			LOG_GPS(DBG_LOW, "MSG server is READY!!");
			cnt = -1;
		} else {
			sleep(5);
			cnt--;
			if (cnt == 0) {
				LOG_GPS(DBG_WARN, "Never connect to MSG Server for 300 secs.");
				return NULL;
			}
		}
	}

	err = msg_open_msg_handle(&msgHandle);

	if (err != MSG_SUCCESS) {
		LOG_GPS(DBG_WARN, "Fail to MsgOpenMsgHandle. Error type = [ %d ]", err);
		return NULL;
	}

	err = msg_reg_sms_message_callback(msgHandle, &_gps_supl_networkinit_smscb, 7275, (void *)server);

	if (err != MSG_SUCCESS) {
		LOG_GPS(DBG_WARN, "Fail to MsgRegSmsMessageCallback. Error type = [ %d ]", err);
		return NULL;
	}

	err = msg_reg_lbs_message_callback(msgHandle, &_gps_supl_networkinit_wappushcb, (void *)server);

	if (err != MSG_SUCCESS) {
		LOG_GPS(DBG_WARN, "Fail to MsgRegLBSMessageCallback. Error type = [ %d ]", err);
		return NULL;
	}

	LOG_GPS(DBG_LOW, "Success to lbs_register_msgfwcb");
	return NULL;

}
#endif

void check_plugin_module(char *module_name)
{
	if (get_replay_enabled() == TRUE) {
		g_strlcpy(module_name, "replay", strlen("replay") + 1);
		LOG_GPS(DBG_LOW, "REPLAY_ENABELD is TRUE");
	} else if (access(BRCM4752_PATH, F_OK) == 0 || access(BRCM47520_PATH, F_OK) == 0) {
		g_strlcpy(module_name, "brcm", strlen("brcm") + 1);
	} else if (access(BRCM47511_PATH, F_OK) == 0) {
		g_strlcpy(module_name, "brcm-legacy", strlen("brcm-legacy") + 1);
	} else if (access(CSR_PATH, F_OK) == 0) {
		g_strlcpy(module_name, "csr", strlen("csr") + 1);
	} else if (access(QCOM8x30_PATH, F_OK) == 0 || access(QCOM9x15_PATH, F_OK) == 0 ||
				access(QCOM8974_PATH, F_OK) == 0 || access(QCOM8210_PATH, F_OK) == 0 ||
				access(QCOM8226_PATH, F_OK) == 0 || access(QCOM8916_PATH, F_OK) == 0) {
		g_strlcpy(module_name, "qcom", strlen("qcom") + 1);
	} else {
		g_strlcpy(module_name, "replay", strlen("replay") + 1);
	}

	LOG_GPS(DBG_LOW, "module name : %s", module_name);
}

static gps_server_t *_initialize_gps_data(void)
{
	g_gps_server = (gps_server_t *) malloc(sizeof(gps_server_t));
	if (g_gps_server == NULL) {
		LOG_GPS(DBG_ERR, "Failed to alloc g_gps_server");
		return NULL;
	}
	memset(g_gps_server, 0x00, sizeof(gps_server_t));

	g_gps_server->session_state = GPS_SESSION_STOPPED;
	g_gps_server->dnet_used = 0;
	g_gps_server->logging_enabled = FALSE;
	g_gps_server->replay_enabled = FALSE;
#ifdef _TIZEN_PUBLIC_
	g_gps_server->msg_thread = 0;
	g_gps_server->popup_thread = 0;
#endif

	memset(&g_gps_server->gps_plugin, 0x00, sizeof(gps_plugin_handler_t));

	if (g_gps_server->pos_data == NULL) {
		g_gps_server->pos_data = (pos_data_t *) malloc(sizeof(pos_data_t));
		if (g_gps_server->pos_data == NULL) {
			LOG_GPS(DBG_ERR, "Failed to alloc g_gps_server->pos_data");
			return NULL;
		} else {
			memset(g_gps_server->pos_data, 0x00, sizeof(pos_data_t));
		}
	}

	if (g_gps_server->batch_data == NULL) {
		g_gps_server->batch_data = (batch_data_t *) malloc(sizeof(batch_data_t));
		if (g_gps_server->batch_data == NULL) {
			LOG_GPS(DBG_ERR, "Failed to alloc g_gps_server->batch_data");
			return NULL;
		} else {
			memset(g_gps_server->batch_data, 0x00, sizeof(batch_data_t));
		}
	}

	if (g_gps_server->sv_data == NULL) {
		g_gps_server->sv_data = (sv_data_t *) malloc(sizeof(sv_data_t));
		if (g_gps_server->sv_data == NULL) {
			LOG_GPS(DBG_ERR, "Failed to alloc g_gps_server->sv_data");
			return NULL;
		} else {
			memset(g_gps_server->sv_data, 0x00, sizeof(sv_data_t));
		}
	}

	if (g_gps_server->nmea_data == NULL) {
		g_gps_server->nmea_data = (nmea_data_t *) malloc(sizeof(nmea_data_t));
		if (g_gps_server->nmea_data == NULL) {
			LOG_GPS(DBG_ERR, "Failed to alloc g_gps_server->nmea_data");
			return NULL;
		} else {
			memset(g_gps_server->nmea_data, 0x00, sizeof(nmea_data_t));
		}
	}

	return g_gps_server;
}

static void _deinitialize_gps_data(void)
{
	if (g_gps_server->pos_data != NULL) {
		free(g_gps_server->pos_data);
		g_gps_server->pos_data = NULL;
	}

	if (g_gps_server->batch_data != NULL) {
		free(g_gps_server->batch_data);
		g_gps_server->batch_data = NULL;
	}

	if (g_gps_server->sv_data != NULL) {
		free(g_gps_server->sv_data);
		g_gps_server->sv_data = NULL;
	}

	if (g_gps_server->nmea_data != NULL) {
		if (g_gps_server->nmea_data->data != NULL) {
			free(g_gps_server->nmea_data->data);
			g_gps_server->nmea_data->data = NULL;
		}
		free(g_gps_server->nmea_data);
		g_gps_server->nmea_data = NULL;
	}

	if (g_gps_server != NULL) {
		free(g_gps_server);
		g_gps_server = NULL;
	}
}

int initialize_server(int argc, char **argv)
{
	FUNC_ENTRANCE_SERVER;

	gps_server_t *server;
	char module_name[16];

	server = _initialize_gps_data();
	if (server == NULL)
		return -1;
	check_plugin_module(module_name);
	_gps_plugin_handler_init(server, module_name);
	_gps_get_nmea_replay_mode(server);
	_gps_notify_params(server);

	if (!load_plugin_module(server->gps_plugin.name, &server->gps_plugin.handle)) {
		LOG_GPS(DBG_ERR, "Fail to load plugin module.");
		return -1;
	}

	if (!get_plugin_module()->init(_gps_server_gps_event_cb, (void *)server)) {
		LOG_GPS(DBG_WARN, "Fail to gps module initialization");
		return -1;
	}

#ifdef _TIZEN_PUBLIC_
	if (pthread_create(&g_gps_server->msg_thread, NULL, _gps_register_msgfwcb, g_gps_server) != 0) {
		LOG_GPS(DBG_WARN, "Can not make pthread......");
	}
#endif

	LOG_GPS(DBG_LOW, "Initialization-gps is completed.");

	if (!nps_load_plugin_module(&g_nps_plugin.handle)) {
		LOG_NPS(DBG_ERR, "Fail to load lbs_server_NPS plugin module.");
		__nps_plugin_handler_deinit();
		return 0; /* TBD: how to deal with this error situation */
	}

	LOG_NPS(DBG_LOW, "Initialization-nps is completed.");

	return 0;
}

int deinitialize_server()
{
	gps_failure_reason_t ReasonCode = GPS_FAILURE_CAUSE_NORMAL;

#ifdef _TIZEN_PUBLIC_
	pthread_join(g_gps_server->msg_thread, (void *)&g_gps_server->msg_thread_status);
#endif

	_gps_ignore_params();

	if (!get_plugin_module()->deinit(&ReasonCode)) {
		LOG_GPS(DBG_WARN, "Fail to gps module de-initialization");
	}
	unload_plugin_module(g_gps_server->gps_plugin.handle);

	_gps_plugin_handler_deinit(g_gps_server);
	_deinitialize_gps_data();

	nps_unload_plugin_module(g_nps_plugin.handle);
	__nps_plugin_handler_deinit();

	return 0;
}

int register_update_callbacks(struct gps_callbacks *gps_callback, void *user_data)
{
	g_update_cb = *gps_callback;
	g_user_data = user_data;
	return 0;
}
