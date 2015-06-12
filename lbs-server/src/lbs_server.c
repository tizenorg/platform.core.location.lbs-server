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
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>

#include <vconf.h>
#include "setting.h"
#include <lbs_dbus_server.h>

#include "gps_plugin_data_types.h"
#include "lbs_server.h"

#include "nps_plugin_module.h"
#include "nps_plugin_intf.h"

#include "server.h"
#include "last_position.h"
#include "debug_util.h"
#include "dump_log.h"


typedef struct {
	/* gps variables */
	pos_data_t position;
	batch_data_t batch;
	sv_data_t satellite;
	nmea_data_t nmea;
	gboolean sv_used;
	gboolean nmea_used;

	gboolean is_gps_running;
	gint gps_client_count;

	/* nps variables */
	NpsManagerHandle handle;
	unsigned long period;
	NpsManagerPositionExt pos;
	NpsManagerLastPositionExt last_pos;
	Plugin_LocationInfo location;

	gboolean is_nps_running;
	gint nps_client_count;
	unsigned char *token;

	/* shared variables */
	GMainLoop *loop;
	GMutex mutex;
	GCond cond;
	LbsStatus status;

	/* dynamic interval update */
	GHashTable *dynamic_interval_table;
	guint *optimized_interval_array;
	guint temp_minimum_interval;
	gboolean is_needed_changing_interval;

	lbs_server_dbus_h lbs_dbus_server;
} lbs_server_s;

typedef struct {
	lbs_server_s *lbs_server;
	int method;
} dynamic_interval_updator_user_data;

static gboolean gps_remove_all_clients(lbs_server_s *lbs_server);

static void __setting_gps_cb(keynode_t *key, gpointer user_data)
{
	LOG_GPS(DBG_LOW, "ENTER >>>");
	lbs_server_s *lbs_server = (lbs_server_s *)user_data;
	int onoff = 0;
	gboolean ret = FALSE;

	setting_get_int(VCONFKEY_LOCATION_ENABLED, &onoff);

	if (onoff == 0) {
		ret = gps_remove_all_clients(lbs_server);
		if (ret == FALSE) {
			LOG_GPS(DBG_LOW, "already removed.");
		}
	}
}

static void nps_set_last_position(NpsManagerPositionExt pos)
{
	LOG_NPS(DBG_LOW, "nps_set_last_position[%d]", pos.timestamp);

	char location[128] = {0,};
	snprintf(location, sizeof(location), "%.6lf;%.6lf;%.2lf;%.2lf;%.2lf;%.2lf;", pos.latitude, pos.longitude, pos.altitude, pos.speed, pos.direction, pos.hor_accuracy);

	setting_set_int(VCONFKEY_LOCATION_NV_LAST_WPS_TIMESTAMP, pos.timestamp);
	setting_set_string(VCONFKEY_LOCATION_NV_LAST_WPS_LOCATION, location);
}

static void nps_set_position(lbs_server_s *lbs_server_nps, NpsManagerPositionExt pos)
{
	LOG_NPS(DBG_LOW, "set position timestamp : %d", pos.timestamp);
	lbs_server_nps->last_pos.timestamp = pos.timestamp;
	setting_set_int(VCONFKEY_LOCATION_LAST_WPS_TIMESTAMP, lbs_server_nps->last_pos.timestamp);

	if ((lbs_server_nps->last_pos.latitude != pos.latitude) || (lbs_server_nps->last_pos.longitude != pos.longitude)) {
		lbs_server_nps->last_pos.latitude = pos.latitude;
		lbs_server_nps->last_pos.longitude = pos.longitude;
		lbs_server_nps->last_pos.altitude = pos.altitude;
		lbs_server_nps->last_pos.hor_accuracy = pos.hor_accuracy;
		lbs_server_nps->last_pos.speed = pos.speed;
		lbs_server_nps->last_pos.direction = pos.direction;

		setting_set_double(VCONFKEY_LOCATION_LAST_WPS_LATITUDE, lbs_server_nps->last_pos.latitude);
		setting_set_double(VCONFKEY_LOCATION_LAST_WPS_LONGITUDE, lbs_server_nps->last_pos.longitude);
		setting_set_double(VCONFKEY_LOCATION_LAST_WPS_ALTITUDE, lbs_server_nps->last_pos.altitude);
		setting_set_double(VCONFKEY_LOCATION_LAST_WPS_SPEED, lbs_server_nps->last_pos.speed);
		setting_set_double(VCONFKEY_LOCATION_LAST_WPS_DIRECTION, lbs_server_nps->last_pos.direction);
		setting_set_double(VCONFKEY_LOCATION_LAST_WPS_HOR_ACCURACY, lbs_server_nps->last_pos.hor_accuracy);
	}

	int last_timestamp = 0;
	setting_get_int(VCONFKEY_LOCATION_NV_LAST_WPS_TIMESTAMP, &last_timestamp);

	LOG_NPS(DBG_LOW, "last_time stamp: %d, pos.timestamp: %d, UPDATE_INTERVAL: %d ", last_timestamp, pos.timestamp, UPDATE_INTERVAL);
	if ((pos.timestamp - last_timestamp) > UPDATE_INTERVAL) {
		nps_set_last_position(pos);
	}

}

static void nps_set_status(lbs_server_s *lbs_server, LbsStatus status)
{
	if (!lbs_server) {
		LOG_NPS(DBG_ERR, "lbs_server is NULL!!");
		return;
	}
	LOG_NPS(DBG_LOW, "nps_set_status[%d]", status);
	if (lbs_server->status == status) {
		LOG_NPS(DBG_ERR, "Donot update NPS status");
		return;
	}

	lbs_server->status = status;

	if (lbs_server->status == LBS_STATUS_AVAILABLE) {
		setting_set_int(VCONFKEY_LOCATION_WPS_STATE, POSITION_CONNECTED);
	} else if (lbs_server->status == LBS_STATUS_ACQUIRING) {
		setting_set_int(VCONFKEY_LOCATION_WPS_STATE, POSITION_SEARCHING);
	} else {
		setting_set_int(VCONFKEY_LOCATION_WPS_STATE, POSITION_OFF);
	}

	if (status != LBS_STATUS_AVAILABLE) {
		lbs_server->pos.fields = LBS_POSITION_FIELDS_NONE;
	}

	lbs_server_emit_status_changed(lbs_server->lbs_dbus_server, LBS_SERVER_METHOD_NPS, status);
}

static void nps_update_position(lbs_server_s *lbs_server_nps, NpsManagerPositionExt pos)
{
	if (!lbs_server_nps) {
		LOG_NPS(DBG_ERR, "lbs-server is NULL!!");
		return;
	}

	GVariant *accuracy = NULL;

	LOG_NPS(DBG_LOW, "nps_update_position");
	lbs_server_nps->pos.fields = pos.fields;
	lbs_server_nps->pos.timestamp = pos.timestamp;
	lbs_server_nps->pos.latitude = pos.latitude;
	lbs_server_nps->pos.longitude = pos.longitude;
	lbs_server_nps->pos.altitude = pos.altitude;
	lbs_server_nps->pos.hor_accuracy = pos.hor_accuracy;
	lbs_server_nps->pos.ver_accuracy = pos.ver_accuracy;

	accuracy = g_variant_ref_sink(g_variant_new("(idd)", lbs_server_nps->pos.acc_level, lbs_server_nps->pos.hor_accuracy, lbs_server_nps->pos.ver_accuracy));

	LOG_NPS(DBG_LOW, "time:%d", lbs_server_nps->pos.timestamp);

	lbs_server_emit_position_changed(lbs_server_nps->lbs_dbus_server, LBS_SERVER_METHOD_NPS,
									lbs_server_nps->pos.fields, lbs_server_nps->pos.timestamp, lbs_server_nps->pos.latitude,
									lbs_server_nps->pos.longitude, lbs_server_nps->pos.altitude, lbs_server_nps->pos.speed,
									lbs_server_nps->pos.direction, 0.0, accuracy);

	g_variant_unref(accuracy);
}

static gboolean nps_report_callback(gpointer user_data)
{
	LOG_NPS(DBG_LOW, "ENTER >>>");
	double	vertical_accuracy = -1.0; /* vertical accuracy's invalid value is -1 */
	Plugin_LocationInfo location;
	memset(&location, 0x00, sizeof(Plugin_LocationInfo));
	lbs_server_s *lbs_server_nps = (lbs_server_s *) user_data;
	if (NULL == lbs_server_nps) {
		LOG_GPS(DBG_ERR, "lbs_server_s is NULL!!");
		return FALSE;
	}
	time_t timestamp;

	g_mutex_lock(&lbs_server_nps->mutex);
	memcpy(&location, &lbs_server_nps->location, sizeof(Plugin_LocationInfo));
	g_mutex_unlock(&lbs_server_nps->mutex);

	time(&timestamp);
	if (timestamp == lbs_server_nps->last_pos.timestamp) {
		return FALSE;
	}

	nps_set_status(lbs_server_nps, LBS_STATUS_AVAILABLE);

	NpsManagerPositionExt pos;
	memset(&pos, 0x00, sizeof(NpsManagerPositionExt));

	pos.timestamp = timestamp;

	if (location.latitude) {
		pos.fields |= LBS_POSITION_EXT_FIELDS_LATITUDE;
		pos.latitude = location.latitude;
	}

	if (location.longitude) {
		pos.fields |= LBS_POSITION_EXT_FIELDS_LONGITUDE;
		pos.longitude = location.longitude;
	}

	if (location.altitude) {
		pos.fields |= LBS_POSITION_EXT_FIELDS_ALTITUDE;
		pos.altitude = location.altitude;
	}

	if (location.speed >= 0) {
		pos.fields |= LBS_POSITION_EXT_FIELDS_SPEED;
		pos.speed = location.speed;
	}
	if (location.bearing >= 0) {
		pos.fields |= LBS_POSITION_EXT_FIELDS_DIRECTION;
		pos.direction = location.bearing;
	}

	pos.acc_level = LBS_ACCURACY_LEVEL_STREET;
	pos.hor_accuracy = location.hpe;
	pos.ver_accuracy = vertical_accuracy;

	nps_update_position(lbs_server_nps, pos);

	nps_set_position(lbs_server_nps, pos);

	return FALSE;
}

static void __nps_callback(void *arg, const Plugin_LocationInfo *location, const void *reserved)
{
	LOG_NPS(DBG_LOW, "ENTER >>>");
	lbs_server_s *lbs_server = (lbs_server_s *)arg;
	if (!lbs_server) {
		LOG_NPS(DBG_ERR, "lbs_server is NULL!!");
		return;
	}

	if (!location) {
		LOG_NPS(DBG_LOW, "NULL is returned from plugin...");
		/* sometimes plugin returns NULL even if it is connected... */
		/*nps_set_status (lbs_server , LBS_STATUS_ACQUIRING); */
		return;
	}

	g_mutex_lock(&lbs_server->mutex);
	memcpy(&lbs_server->location, location, sizeof(Plugin_LocationInfo));
	g_mutex_unlock(&lbs_server->mutex);

	g_main_context_invoke(NULL, nps_report_callback, arg);
}

static void _nps_token_callback(void *arg, const unsigned char *token, unsigned tokenSize)
{
	LOG_NPS(DBG_LOW, "ENTER >>>");
	lbs_server_s *lbs_server_nps = (lbs_server_s *)arg;
	if (NULL == lbs_server_nps) {
		LOG_NPS(DBG_ERR, "lbs-server is NULL!!");
		return;
	}

	if (token == NULL) {
		LOG_NPS(DBG_ERR, "*** no token to save\n");
	} else {
		/* save the token in persistent storage */
		g_mutex_lock(&lbs_server_nps->mutex);
		lbs_server_nps->token = g_new0(unsigned char, tokenSize);
		if (lbs_server_nps->token) {
			memcpy(lbs_server_nps->token, token, tokenSize);
		}
		g_mutex_unlock(&lbs_server_nps->mutex);
	}
	LOG_NPS(DBG_LOW, "_nps_token_callback ended");
}

static void _network_enabled_cb(keynode_t *key, void *user_data)
{
	LOG_GPS(DBG_LOW, "ENTER >>>");
	int enabled = 0;

	setting_get_int(VCONFKEY_LOCATION_NETWORK_ENABLED, &enabled);

	/* This is vestige of nps-server
	lbs_server_s *lbs_server_nps = (lbs_server_s *)user_data;

	if (enabled == 0) {
		if (lbs_server_nps->loop != NULL) {
			g_main_loop_quit(lbs_server_nps->loop);
		}
	}
	*/
}
static gboolean nps_offline_location(lbs_server_s *lbs_server)
{
	LOG_NPS(DBG_LOW, "ENTER >>>");
	if (NULL == lbs_server) {
		LOG_GPS(DBG_ERR, "lbs-server is NULL!!");
		return FALSE;
	}
	char key[NPS_UNIQUE_ID_LEN] = { 0, };

	if (setting_get_unique_id(key)) {
		LOG_NPS(DBG_LOW, "key = %s", key);
		get_nps_plugin_module()->getOfflineToken((unsigned char *)key, NPS_UNIQUE_ID_LEN,
				_nps_token_callback, lbs_server);
		if (lbs_server->token != NULL) {
			LOG_NPS(DBG_LOW, "Token = %s", lbs_server->token);
			int ret = get_nps_plugin_module()->offlineLocation((unsigned char *)key,
					NPS_UNIQUE_ID_LEN, lbs_server->token, 0,
					__nps_callback,	lbs_server);
			if (ret) {
				if (lbs_server->is_nps_running) {
					LOG_NPS(DBG_LOW, "offlineLocation() called nps_callback successfully.");
					return TRUE;
				}
			}
		} else {
			LOG_NPS(DBG_ERR, "getOfflineToken(): Token is NULL");
		}
	} else {
		LOG_NPS(DBG_ERR, "lbs_get_unique_id() failed.");
	}
	return TRUE;
}

static void __nps_cancel_callback(void *arg)
{
	LOG_NPS(DBG_LOW, "__nps_cancel_callback");
	lbs_server_s *lbs_server = (lbs_server_s *) arg;
	if (!lbs_server) {
		LOG_NPS(DBG_ERR, "lbs-server is NULL!!");
		return;
	}

	g_mutex_lock(&lbs_server->mutex);
	lbs_server->is_nps_running = FALSE;
	g_cond_signal(&lbs_server->cond);
	g_mutex_unlock(&lbs_server->mutex);
}

static void start_batch_tracking(lbs_server_s *lbs_server, int batch_interval, int batch_period)
{
	g_mutex_lock(&lbs_server->mutex);
	lbs_server->gps_client_count++;
	g_mutex_unlock(&lbs_server->mutex);

	if (lbs_server->is_gps_running == TRUE) {
		LOG_GPS(DBG_LOW, "Batch: gps is already running");
		return;
	}
	LOG_GPS(DBG_LOW, "Batch: start_tracking GPS");
	lbs_server->status = LBS_STATUS_ACQUIRING;

	if (request_start_batch_session(batch_interval, batch_period) == TRUE) {
		g_mutex_lock(&lbs_server->mutex);
		lbs_server->is_gps_running = TRUE;
		g_mutex_unlock(&lbs_server->mutex);

		lbs_server->is_needed_changing_interval = FALSE;

		/* ADD notify */
		setting_notify_key_changed(VCONFKEY_LOCATION_ENABLED, __setting_gps_cb, lbs_server);
	} else {
		LOG_GPS(DBG_ERR, "Batch: Fail to request_start_batch_session");
	}
}

static void stop_batch_tracking(lbs_server_s *lbs_server)
{
	LOG_GPS(DBG_LOW, "ENTER >>>");

	g_mutex_lock(&lbs_server->mutex);
	lbs_server->gps_client_count--;
	g_mutex_unlock(&lbs_server->mutex);

	if (lbs_server->is_gps_running == FALSE) {
		LOG_GPS(DBG_LOW, "Batch: gps- is already stopped");
		return;
	}

	if (lbs_server->gps_client_count <= 0) {
		g_mutex_lock(&lbs_server->mutex);
		lbs_server->gps_client_count = 0;

		if (request_stop_batch_session() == TRUE) {
			lbs_server->is_gps_running = FALSE;
			lbs_server->sv_used = FALSE;
			/* remove notify */
			setting_ignore_key_changed(VCONFKEY_LOCATION_ENABLED, __setting_gps_cb);
			g_mutex_unlock(&lbs_server->mutex);
		}
	}

	lbs_server->status = LBS_STATUS_UNAVAILABLE;
	lbs_server_emit_status_changed(lbs_server->lbs_dbus_server, LBS_SERVER_METHOD_GPS, LBS_STATUS_UNAVAILABLE);
}

static void start_tracking(lbs_server_s *lbs_server, lbs_server_method_e method)
{
	switch (method) {
		case LBS_SERVER_METHOD_GPS:

			g_mutex_lock(&lbs_server->mutex);
			lbs_server->gps_client_count++;
			g_mutex_unlock(&lbs_server->mutex);

			if (lbs_server->is_gps_running == TRUE) {
				LOG_GPS(DBG_LOW, "gps is already running");
				return;
			}
			LOG_GPS(DBG_LOW, "start_tracking GPS");
			lbs_server->status = LBS_STATUS_ACQUIRING;

			if (request_start_session((int)(lbs_server->optimized_interval_array[method])) == TRUE) {
				g_mutex_lock(&lbs_server->mutex);
				lbs_server->is_gps_running = TRUE;
				g_mutex_unlock(&lbs_server->mutex);

				lbs_server->is_needed_changing_interval = FALSE;

				/* ADD notify */
				setting_notify_key_changed(VCONFKEY_LOCATION_ENABLED, __setting_gps_cb, lbs_server);
			} else {
				LOG_GPS(DBG_ERR, "Fail to request_start_session");
			}

			break;

		case LBS_SERVER_METHOD_NPS:
			g_mutex_lock(&lbs_server->mutex);
			lbs_server->nps_client_count++;
			g_mutex_unlock(&lbs_server->mutex);

			if (lbs_server->is_nps_running == TRUE) {
				LOG_NPS(DBG_LOW, "nps is already running");
				return;
			}

			LOG_NPS(DBG_LOW, "start_tracking - LBS_SERVER_METHOD_NPS");
			nps_set_status(lbs_server, LBS_STATUS_ACQUIRING);

			void *handle_str = NULL;
			int ret = get_nps_plugin_module()->start(lbs_server->period, __nps_callback, lbs_server, &(handle_str));
			LOG_NPS(DBG_LOW, "after get_nps_plugin_module()->location");

			if (ret) {
				lbs_server->handle = handle_str;
				g_mutex_lock(&lbs_server->mutex);
				lbs_server->is_nps_running = TRUE;
				g_mutex_unlock(&lbs_server->mutex);
				vconf_ignore_key_changed(VCONFKEY_LOCATION_NETWORK_ENABLED, _network_enabled_cb);
				return;
			}

			if (nps_is_dummy_plugin_module() != TRUE) {
				nps_offline_location(lbs_server);
			}

			LOG_NPS(DBG_ERR, "FAILS WPS START");
			nps_set_status(lbs_server, LBS_STATUS_ERROR);
			break;

		default:
			LOG_GPS(DBG_LOW, "start_tracking Invalid");
	}

}

static gboolean nps_stop(lbs_server_s *lbs_server_nps)
{
	LOG_NPS(DBG_LOW, "ENTER >>>");
	if (!lbs_server_nps) {
		LOG_NPS(DBG_ERR, "lbs-server is NULL!!");
		return FALSE;
	}

	int ret = get_nps_plugin_module()->stop(lbs_server_nps->handle, __nps_cancel_callback, lbs_server_nps);
	if (ret) {
		g_mutex_lock(&lbs_server_nps->mutex);
		while (lbs_server_nps->is_nps_running) {
			g_cond_wait(&lbs_server_nps->cond, &lbs_server_nps->mutex);
		}
		lbs_server_nps->is_nps_running = FALSE;
		g_mutex_unlock(&lbs_server_nps->mutex);
	}
	/* This is vestige of nps-server
	else {
		if (lbs_server_nps->loop != NULL) {
			LOG_NPS(DBG_ERR, "module->stop() is failed. nps is cloesed.");
			g_main_loop_quit(lbs_server_nps->loop);
			return FALSE;
		}
	}
	*/

	nps_set_status(lbs_server_nps, LBS_STATUS_UNAVAILABLE);

	return TRUE;
}

static void stop_tracking(lbs_server_s *lbs_server, lbs_server_method_e method)
{
	switch (method) {
		case LBS_SERVER_METHOD_GPS:
			LOG_GPS(DBG_LOW, "stop_tracking GPS");

			gps_set_last_position(&lbs_server->position);

			g_mutex_lock(&lbs_server->mutex);
			lbs_server->gps_client_count--;
			g_mutex_unlock(&lbs_server->mutex);

			if (lbs_server->is_gps_running == FALSE) {
				LOG_GPS(DBG_LOW, "gps- is already stopped");
				return;
			}

			if (lbs_server->gps_client_count <= 0) {
				g_mutex_lock(&lbs_server->mutex);
				lbs_server->gps_client_count = 0;

				if (request_stop_session() == TRUE) {
					lbs_server->is_gps_running = FALSE;
					lbs_server->sv_used = FALSE;
					/* remove notify */
					setting_ignore_key_changed(VCONFKEY_LOCATION_ENABLED, __setting_gps_cb);
					g_mutex_unlock(&lbs_server->mutex);
				}
			}

			lbs_server->status = LBS_STATUS_UNAVAILABLE;
			lbs_server_emit_status_changed(lbs_server->lbs_dbus_server, LBS_SERVER_METHOD_GPS,
											LBS_STATUS_UNAVAILABLE);

			break;
		case LBS_SERVER_METHOD_NPS:
			LOG_NPS(DBG_LOW, "stop_tracking NPS");

			g_mutex_lock(&lbs_server->mutex);
			lbs_server->nps_client_count--;
			g_mutex_unlock(&lbs_server->mutex);

			if (lbs_server->is_nps_running == FALSE) {
				LOG_NPS(DBG_LOW, "nps- is already stopped");
				return;
			}

			if (lbs_server->nps_client_count <= 0) {
				g_mutex_lock(&lbs_server->mutex);
				lbs_server->nps_client_count = 0;
				g_mutex_unlock(&lbs_server->mutex);

				LOG_NPS(DBG_LOW, "lbs_server_nps Normal stop");
				nps_stop(lbs_server);
			}

			break;
		default:
			LOG_GPS(DBG_LOW, "stop_tracking Invalid");
	}
}

static void update_dynamic_interval_table_foreach_cb(gpointer key, gpointer value, gpointer userdata)
{
	guint *interval_array = (guint *)value;
	dynamic_interval_updator_user_data *updator_ud = (dynamic_interval_updator_user_data *)userdata;
	lbs_server_s *lbs_server = updator_ud->lbs_server;
	int method = updator_ud->method;

	LOG_GPS(DBG_LOW, "foreach dynamic-interval. method:[%d], key:[%s]-interval:[%u], current optimized interval [%u]", method, (char *)key, interval_array[method], lbs_server->optimized_interval_array[method]);
	if ((lbs_server->temp_minimum_interval > interval_array[method])) {
		lbs_server->temp_minimum_interval = interval_array[method];
	}
}

static gboolean update_pos_tracking_interval(lbs_server_interval_manipulation_type type, const gchar *client, int method, guint interval, gpointer userdata)
{
	LOG_GPS(DBG_LOW, "ENTER >>>");
	if (userdata == NULL) return FALSE;
	if (client == NULL) {
		LOG_GPS(DBG_ERR, "client is NULL");
		return FALSE;
	}

	if (method < LBS_SERVER_METHOD_GPS || method >= LBS_SERVER_METHOD_SIZE) {
		LOG_GPS(DBG_ERR, "unavailable method [%d]", method);
		return FALSE;
	}

	gboolean ret_val = FALSE;
	lbs_server_s *lbs_server = (lbs_server_s *)userdata;

	/* manipulate logic for dynamic-interval hash table */
	switch (type) {
		case LBS_SERVER_INTERVAL_ADD: {
				LOG_GPS(DBG_LOW, "ADD, client[%s], method[%d], interval[%u]", client, method, interval);
				gchar *client_cpy = NULL;
				client_cpy = g_strdup(client);

				guint *interval_array = (guint *) g_hash_table_lookup(lbs_server->dynamic_interval_table, client_cpy);
				if (!interval_array) {
					LOG_GPS(DBG_LOW, "first add key[%s] to interval-table", client);
					interval_array = (guint *)g_malloc0(LBS_SERVER_METHOD_SIZE * sizeof(guint));
					if (!interval_array) {
						LOG_GPS(DBG_ERR, "interval_array is NULL");
						g_free(client_cpy);
						return FALSE;
					}
					g_hash_table_insert(lbs_server->dynamic_interval_table, (gpointer)client_cpy, (gpointer)interval_array);
				}
				interval_array[method] = interval;
				lbs_server->temp_minimum_interval = interval;
				LOG_GPS(DBG_LOW, "ADD done");
				break;
			}

		case LBS_SERVER_INTERVAL_REMOVE: {
				LOG_GPS(DBG_LOW, "REMOVE, client[%s], method[%d]", client, method);
				lbs_server->temp_minimum_interval = 120; /* interval max value */
				guint *interval_array = (guint *) g_hash_table_lookup(lbs_server->dynamic_interval_table, client);
				if (!interval_array) {
					LOG_GPS(DBG_INFO, "Client[%s] Method[%d] is already removed from interval-table", client, method);
					break;
				}
				LOG_GPS(DBG_LOW, "Found interval_array[%d](%p):[%u] from interval-table", method, interval_array, interval_array[method]);
				interval_array[method] = 0;

				int i = 0;
				guint interval_each = 0;
				gboolean is_need_remove_client_from_table = TRUE;
				for (i = 0; i < LBS_SERVER_METHOD_SIZE; i++) {
					interval_each = interval_array[i];
					if (interval_each != 0) {
						LOG_GPS(DBG_LOW, "[%s] method[%d]'s interval is not zero - interval: %d. It will not be removed.", client, i, interval_each);
						is_need_remove_client_from_table = FALSE;
						break;
					}
				}

				if (is_need_remove_client_from_table) {
					LOG_GPS(DBG_LOW, "is_need_remove_client_from_table is TRUE");
					if (!g_hash_table_remove(lbs_server->dynamic_interval_table, client)) {
						LOG_GPS(DBG_ERR, "g_hash_table_remove is failed.");
					}
					LOG_GPS(DBG_LOW, "REMOVE done.");
				}
				break;
			}

		case LBS_SERVER_INTERVAL_UPDATE: {
				LOG_GPS(DBG_LOW, "UPDATE client[%s], method[%d], interval[%u]", client, method, interval);
				guint *interval_array = (guint *) g_hash_table_lookup(lbs_server->dynamic_interval_table, client);
				if (!interval_array) {
					LOG_GPS(DBG_LOW, "Client[%s] is not exist in interval-table", client, method);
					break;
				}
				interval_array[method] = interval;
				lbs_server->temp_minimum_interval = interval;
				LOG_GPS(DBG_LOW, "UPDATE done.");
				break;
			}

		default: {
				LOG_GPS(DBG_ERR, "unhandled interval-update type");
				return FALSE;
			}
	}

	/* update logic for optimized-interval value */
	if (g_hash_table_size(lbs_server->dynamic_interval_table) == 0) {
		LOG_GPS(DBG_LOW, "dynamic_interval_table size is zero. It will be updated all value as 0");
		int i = 0;
		for (i = 0; i < LBS_SERVER_METHOD_SIZE; i++) {
			lbs_server->optimized_interval_array[i] = 0;
		}
		ret_val = FALSE;
	} else {
		LOG_GPS(DBG_LOW, "dynamic_interval_table size is not zero.");

		dynamic_interval_updator_user_data updator_user_data;
		updator_user_data.lbs_server = lbs_server;
		updator_user_data.method = method;

		g_hash_table_foreach(lbs_server->dynamic_interval_table, (GHFunc)update_dynamic_interval_table_foreach_cb, (gpointer)&updator_user_data);

		if (lbs_server->optimized_interval_array[method] != lbs_server->temp_minimum_interval) {
			LOG_GPS(DBG_INFO, "Changing optimized_interval value [%u -> %u] for method [%d]",
					lbs_server->optimized_interval_array[method], lbs_server->temp_minimum_interval, method);
			lbs_server->optimized_interval_array[method] = lbs_server->temp_minimum_interval;

			/* need to send message to provider device */
			lbs_server->is_needed_changing_interval = TRUE;
		}

		if (lbs_server->is_needed_changing_interval) {
			ret_val = TRUE;
		} else {
			ret_val = FALSE;
		}
	}
	LOG_GPS(DBG_LOW, "update_pos_tracking_interval done.");
	return ret_val;
}

static void request_change_pos_update_interval(int method, gpointer userdata)
{
	LOG_GPS(DBG_LOW, "ENTER >>>");
	if (!userdata) return;

	lbs_server_s *lbs_server = (lbs_server_s *)userdata;
	switch (method) {
		case LBS_SERVER_METHOD_GPS:
			request_change_pos_update_interval_standalone_gps(lbs_server->optimized_interval_array[method]);
			break;
		default:
			break;
	}
}

static void get_nmea(int *timestamp, gchar **nmea_data, gpointer userdata)
{
	LOG_GPS(DBG_LOW, "ENTER >>>");
	if (!userdata) return;

	get_nmea_from_server(timestamp, nmea_data);

	LOG_GPS(DBG_LOW, "timestmap: %d, nmea_data: %s", *timestamp, *nmea_data);
}

static void set_options(GVariant *options, const gchar *client, gpointer userdata)
{
	LOG_GPS(DBG_LOW, "ENTER >>>");
	lbs_server_s *lbs_server = (lbs_server_s *)userdata;

	GVariantIter iter;
	gchar *key = NULL;
	GVariant *value = NULL;
	gboolean ret = FALSE;
#ifdef _TIZEN_PUBLIC_
	char *msg_header = NULL;
	char *msg_body = NULL;
	int size = 0;
#endif
	gsize length = 0;
	char *option = NULL;
	char *app_id = NULL;
	guint interval = 0;

	lbs_server_method_e method = 0;

	g_variant_iter_init(&iter, options);
	ret = g_variant_iter_next(&iter, "{&sv}", &key, &value);
	if (ret == FALSE) {
		LOG_GPS(DBG_ERR, "Invalid GVariant");
		return;
	}
	if (!g_strcmp0(key, "CMD")) {
		LOG_GPS(DBG_LOW, "set_options [%s]", g_variant_get_string(value, &length));
		if (!g_strcmp0(g_variant_get_string(value, &length), "START")) {

			while (g_variant_iter_next(&iter, "{&sv}", &key, &value)) {
				if (!g_strcmp0(key, "METHOD")) {
					method = g_variant_get_int32(value);
					LOG_GPS(DBG_LOW, "METHOD [%d]", method);
				}

				if (!g_strcmp0(key, "INTERVAL")) {
					interval = g_variant_get_uint32(value);
					LOG_GPS(DBG_LOW, "INTERVAL [%u]", interval);
				}

				if (!g_strcmp0(key, "APP_ID")) {
					app_id = g_variant_dup_string(value, &length);
					LOG_GPS(DBG_LOW, "APP_ID [%s]", app_id);
				}
			}

			if (client) {
				update_pos_tracking_interval(LBS_SERVER_INTERVAL_ADD, client, method, interval, lbs_server);
			}

			if (app_id) {
				if (LBS_SERVER_METHOD_GPS == method) {
					gps_dump_log("START GPS", app_id);
				}
				free(app_id);
			}

			start_tracking(lbs_server, method);

			if (lbs_server->is_needed_changing_interval) {
				lbs_server->is_needed_changing_interval = FALSE;
				request_change_pos_update_interval(method, (gpointer)lbs_server);
			}
		} else if (!g_strcmp0(g_variant_get_string(value, &length), "STOP")) {

			while (g_variant_iter_next(&iter, "{&sv}", &key, &value)) {
				if (!g_strcmp0(key, "METHOD")) {
					method = g_variant_get_int32(value);
					LOG_GPS(DBG_LOW, "METHOD [%d]", method);
				}

				if (!g_strcmp0(key, "APP_ID")) {
					app_id = g_variant_dup_string(value, &length);
					LOG_GPS(DBG_LOW, "APP_ID [%s]", app_id);
				}
			}

			if (client) {
				update_pos_tracking_interval(LBS_SERVER_INTERVAL_REMOVE, client, method, interval, lbs_server);
			}

			if (app_id) {
				if (LBS_SERVER_METHOD_GPS == method) {
					gps_dump_log("STOP GPS", app_id);
				}
				free(app_id);
			}

			stop_tracking(lbs_server, method);

			if (lbs_server->is_needed_changing_interval) {
				lbs_server->is_needed_changing_interval = FALSE;
				request_change_pos_update_interval(method, (gpointer)lbs_server);
			}
		} else if (!g_strcmp0(g_variant_get_string(value, &length), "START_BATCH")) {

			int batch_interval = 0;
			int batch_period = 0;
			while (g_variant_iter_next(&iter, "{&sv}", &key, &value)) {

				if (!g_strcmp0(key, "BATCH_INTERVAL")) {
					batch_interval = g_variant_get_int32(value);
					LOG_GPS(DBG_LOW, "BATCH_INTERVAL [%d]", batch_interval);
				} else if (!g_strcmp0(key, "BATCH_PERIOD")) {
					batch_period = g_variant_get_int32(value);
					LOG_GPS(DBG_LOW, "BATCH_PERIOD [%d]", batch_period);
				}
			}

			if (client) {
				update_pos_tracking_interval(LBS_SERVER_INTERVAL_ADD, client, method, batch_interval, lbs_server);
			}

			start_batch_tracking(lbs_server, batch_interval, batch_period);

			if (lbs_server->is_needed_changing_interval) {
				lbs_server->is_needed_changing_interval = FALSE;
				request_change_pos_update_interval(method, (gpointer)lbs_server);
			}

		} else if (!g_strcmp0(g_variant_get_string(value, &length), "STOP_BATCH")) {

			if (client) {
				update_pos_tracking_interval(LBS_SERVER_INTERVAL_REMOVE, client, method, interval, lbs_server);
			}

			stop_batch_tracking(lbs_server);

			if (lbs_server->is_needed_changing_interval) {
				lbs_server->is_needed_changing_interval = FALSE;
				request_change_pos_update_interval(method, (gpointer)lbs_server);
			}
		}
#ifdef _TIZEN_PUBLIC_
	} else if (!g_strcmp0(g_variant_get_string(value, &length), "SUPLNI")) {
		while (g_variant_iter_next(&iter, "{&sv}", &key, &value)) {
			if (!g_strcmp0(key, "HEADER")) {
				msg_header = g_variant_dup_string(value, &length);
			} else if (!g_strcmp0(key, "BODY")) {
				size = (int) g_variant_get_size(value);
				msg_body = (char *) g_malloc0(sizeof(char) * size);
				memcpy(msg_body, g_variant_get_data(value), size);
			} else if (!g_strcmp0(key, "SIZE")) {
				size = (int) g_variant_get_int32(value);
			}
		}
		request_supl_ni_session(msg_header, msg_body, size);
		if (msg_header) g_free(msg_header);
		if (msg_body) g_free(msg_body);
	}
#endif
	else if (!g_strcmp0(g_variant_get_string(value, &length), "SET:OPT")) {
		LOG_GPS(DBG_LOW, "SET:OPT is called");
		gboolean is_update_interval = FALSE, is_update_interval_method = FALSE;

		while (g_variant_iter_next(&iter, "{&sv}", &key, &value)) {

			if (!g_strcmp0(key, "OPTION")) {
				option = g_variant_dup_string(value, &length);
				LOG_GPS(DBG_ERR, "option [%s]", option);

				if (!g_strcmp0(option, "DELGPS")) {
					if (request_delete_gps_data() != TRUE) {
						LOG_GPS(DBG_ERR, "Fail to request_delete_gps_data");
					}
				} else if (!g_strcmp0(option, "USE_SV")) {
					g_mutex_lock(&lbs_server->mutex);
					if (lbs_server->sv_used == FALSE)
						lbs_server->sv_used = TRUE;
					g_mutex_unlock(&lbs_server->mutex);
				}
				g_free(option);
			}

			if (!g_strcmp0(key, "METHOD")) {
				method = g_variant_get_int32(value);
				LOG_GPS(DBG_LOW, "METHOD [%d]", method);
				is_update_interval_method = TRUE;
			}

			if (!g_strcmp0(key, "INTERVAL_UPDATE")) {
				interval = g_variant_get_uint32(value);
				LOG_GPS(DBG_LOW, "INTERVAL_UPDATE [%u]", interval);
				is_update_interval = TRUE;
			}
		}

		if (is_update_interval && is_update_interval_method && client) {
			update_pos_tracking_interval(LBS_SERVER_INTERVAL_UPDATE, client, method, interval, lbs_server);
			if (lbs_server->is_needed_changing_interval) {
				lbs_server->is_needed_changing_interval = FALSE;
				request_change_pos_update_interval(method, (gpointer)lbs_server);
			}
		}
	}
}
}

static gboolean gps_remove_all_clients(lbs_server_s *lbs_server)
{
	LOG_GPS(DBG_LOW, "remove_all_clients GPS");
	if (lbs_server->gps_client_count <= 0) {
		lbs_server->gps_client_count = 0;
		return FALSE;
	}

	lbs_server->gps_client_count = 0;
	stop_tracking(lbs_server, LBS_SERVER_METHOD_GPS);

	return TRUE;
}

static void shutdown(gpointer userdata, gboolean *shutdown_arr)
{
	LOG_GPS(DBG_LOW, "shutdown callback gps:%d nps:%d", shutdown_arr[LBS_SERVER_METHOD_GPS], shutdown_arr[LBS_SERVER_METHOD_NPS]);
	lbs_server_s *lbs_server = (lbs_server_s *)userdata;

	if (shutdown_arr[LBS_SERVER_METHOD_GPS]) {
		LOG_GPS(DBG_LOW, "-> shutdown GPS");
		if (lbs_server->is_gps_running) {
			if (gps_remove_all_clients(lbs_server)) {
				LOG_GPS(DBG_ERR, "<<<< Abnormal shutdown >>>>");
			}
		}
	}

	if (shutdown_arr[LBS_SERVER_METHOD_NPS]) {
		LOG_NPS(DBG_LOW, "-> shutdown NPS");
		if (lbs_server->is_nps_running) {
			LOG_NPS(DBG_ERR, "lbs_server_nps is running. nps_stop");
			nps_stop(lbs_server);
		}
	}

	/*
	int enabled = 0;
	setting_get_int(VCONFKEY_LOCATION_NETWORK_ENABLED, &enabled);
	if (enabled == 0) {
		if (lbs_server->loop != NULL) {
			g_main_loop_quit(lbs_server->loop);
		}
	} else {
		if (vconf_notify_key_changed(VCONFKEY_LOCATION_NETWORK_ENABLED, _network_enabled_cb, lbs_server)) {
			LOG_NPS(DBG_ERR, "fail to notify VCONFKEY_LOCATION_NETWORK_ENABLED");
		}
	}
	 */
}

static void gps_update_position_cb(pos_data_t *pos, gps_error_t error, void *user_data)
{
	LOG_GPS(DBG_LOW, "ENTER >>>");

	GVariant *accuracy = NULL;
	LbsPositionExtFields fields;

	lbs_server_s *lbs_server = (lbs_server_s *)(user_data);

	memcpy(&lbs_server->position, pos, sizeof(pos_data_t));

	if (lbs_server->status != LBS_STATUS_AVAILABLE) {
		lbs_server_emit_status_changed(lbs_server->lbs_dbus_server, LBS_SERVER_METHOD_GPS, LBS_STATUS_AVAILABLE);
		lbs_server->status = LBS_STATUS_AVAILABLE;
	}

	fields = (LBS_POSITION_EXT_FIELDS_LATITUDE | LBS_POSITION_EXT_FIELDS_LONGITUDE | LBS_POSITION_EXT_FIELDS_ALTITUDE | LBS_POSITION_EXT_FIELDS_SPEED | LBS_POSITION_EXT_FIELDS_DIRECTION);

	accuracy = g_variant_new("(idd)", LBS_ACCURACY_LEVEL_DETAILED, pos->hor_accuracy, pos->ver_accuracy);
	if (NULL == accuracy) {
		LOG_GPS(DBG_LOW, "accuracy is NULL");
	}

	lbs_server_emit_position_changed(lbs_server->lbs_dbus_server, LBS_SERVER_METHOD_GPS,
									 fields, pos->timestamp, pos->latitude, pos->longitude, pos->altitude,
									 pos->speed, pos->bearing, 0.0, accuracy);
}

static void gps_update_batch_cb(batch_data_t *batch, void *user_data)
{
	LOG_GPS(DBG_LOW, "ENTER >>>");
	lbs_server_s *lbs_server = (lbs_server_s *)(user_data);
	memcpy(&lbs_server->batch, batch, sizeof(batch_data_t));

	if (lbs_server->status != LBS_STATUS_AVAILABLE) {
		lbs_server_emit_status_changed(lbs_server->lbs_dbus_server, LBS_SERVER_METHOD_GPS, LBS_STATUS_AVAILABLE);
		lbs_server->status = LBS_STATUS_AVAILABLE;
	}

	lbs_server_emit_batch_changed(lbs_server->lbs_dbus_server, batch->num_of_location);
}

static void gps_update_satellite_cb(sv_data_t *sv, void *user_data)
{
	lbs_server_s *lbs_server = (lbs_server_s *)(user_data);
	if (lbs_server->sv_used == FALSE) {
		/*LOG_GPS(DBG_LOW, "sv_used is FALSE"); */
		return;
	}

	LOG_GPS(DBG_LOW, "ENTER >>>");
	int index;
	int timestamp = 0;
	int satellite_used = 0;
	GVariant *used_prn = NULL;
	GVariantBuilder *used_prn_builder = NULL;
	GVariant *satellite_info = NULL;
	GVariantBuilder *satellite_info_builder = NULL;

	memcpy(&lbs_server->satellite, sv, sizeof(sv_data_t));
	timestamp = sv->timestamp;

	used_prn_builder = g_variant_builder_new(G_VARIANT_TYPE("ai"));
	for (index = 0; index < sv->num_of_sat; ++index) {
		if (sv->sat[index].used) {
			g_variant_builder_add(used_prn_builder, "i", sv->sat[index].prn);
			++satellite_used;
		}
	}
	used_prn = g_variant_builder_end(used_prn_builder);

	satellite_info_builder = g_variant_builder_new(G_VARIANT_TYPE("a(iiii)"));
	for (index = 0; index < sv->num_of_sat; ++index) {
		g_variant_builder_add(satellite_info_builder, "(iiii)", sv->sat[index].prn,
							sv->sat[index].elevation, sv->sat[index].azimuth, sv->sat[index].snr);
	}
	satellite_info = g_variant_builder_end(satellite_info_builder);

	lbs_server_emit_satellite_changed(lbs_server->lbs_dbus_server, timestamp, satellite_used, sv->num_of_sat,
									used_prn, satellite_info);
}

static void gps_update_nmea_cb(nmea_data_t *nmea, void *user_data)
{
	lbs_server_s *lbs_server = (lbs_server_s *)(user_data);

	if (lbs_server->nmea.data) {
		g_free(lbs_server->nmea.data);
		lbs_server->nmea.len = 0;
		lbs_server->nmea.data = NULL;
	}
	lbs_server->nmea.timestamp = lbs_server->position.timestamp;
	lbs_server->nmea.data = g_malloc(nmea->len + 1);
	g_return_if_fail(lbs_server->nmea.data);

	g_memmove(lbs_server->nmea.data, nmea->data, nmea->len);
	lbs_server->nmea.data[nmea->len] = '\0';
	lbs_server->nmea.len = nmea->len;

	if (lbs_server->nmea_used == FALSE) {
		/*LOG_GPS(DBG_LOW, "nmea_used is FALSE"); */
		return;
	}
	LOG_GPS(DBG_LOW, "[%d] %s", lbs_server->nmea.timestamp, lbs_server->nmea.data);
	lbs_server_emit_nmea_changed(lbs_server->lbs_dbus_server, lbs_server->nmea.timestamp, lbs_server->nmea.data);
}

int gps_update_geofence_transition(int geofence_id, geofence_zone_state_t transition, double latitude, double longitude, double altitude, double speed, double bearing, double hor_accuracy, void *data)
{
	lbs_server_s *lbs_server = (lbs_server_s *)data;
	lbs_server_emit_gps_geofence_changed(lbs_server->lbs_dbus_server, geofence_id, transition, latitude, longitude, altitude, speed, bearing, hor_accuracy);
	return 0;
}

int gps_update_geofence_service_status(int status, void *data)
{
	lbs_server_s *lbs_server = (lbs_server_s *)data;
	lbs_server_emit_gps_geofence_status_changed(lbs_server->lbs_dbus_server, status);
	return 0;
}

static void add_fence(gint fence_id, gdouble latitude, gdouble longitude, gint radius, gint last_state, gint monitor_states, gint notification_responsiveness, gint unknown_timer, gpointer userdata)
{
	request_add_geofence(fence_id, latitude, longitude, radius, last_state, monitor_states, notification_responsiveness, unknown_timer);
}

static void remove_fence(gint fence_id, gpointer userdata)
{
	request_delete_geofence(fence_id);
}

static void pause_fence(gint fence_id, gpointer userdata)
{
	request_pause_geofence(fence_id);
}

static void resume_fence(gint fence_id, gint monitor_states, gpointer userdata)
{
	request_resume_geofence(fence_id, monitor_states);
}

static void nps_init(lbs_server_s *lbs_server_nps);

static void lbs_server_init(lbs_server_s *lbs_server)
{
	LOG_GPS(DBG_LOW, "lbs_server_init");

	lbs_server->status = LBS_STATUS_UNAVAILABLE;
	g_mutex_init(&lbs_server->mutex);

	memset(&lbs_server->position, 0x00, sizeof(pos_data_t));
	memset(&lbs_server->satellite, 0x00, sizeof(sv_data_t));
	memset(&lbs_server->nmea, 0x00, sizeof(nmea_data_t));

	lbs_server->is_gps_running = FALSE;
	lbs_server->sv_used = FALSE;
	lbs_server->nmea_used = FALSE;
	lbs_server->gps_client_count = 0;

	nps_init(lbs_server);

	/* create resource for dynamic-interval */
	lbs_server->dynamic_interval_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	lbs_server->optimized_interval_array = (guint *)g_malloc0(LBS_SERVER_METHOD_SIZE * sizeof(guint));
	lbs_server->is_needed_changing_interval = FALSE;
}

static void nps_get_last_position(lbs_server_s *lbs_server_nps)
{
	int timestamp;
	char location[128] = {0,};
	char *last_location[MAX_NPS_LOC_ITEM] = {0,};
	char *last = NULL;
	char *str = NULL;
	int index = 0;

	setting_get_int(VCONFKEY_LOCATION_NV_LAST_WPS_TIMESTAMP, &timestamp);
	str = setting_get_string(VCONFKEY_LOCATION_NV_LAST_WPS_LOCATION);
	if (str == NULL) {
		return;
	}
	snprintf(location, sizeof(location), "%s", str);
	free(str);

	last_location[index] = (char *)strtok_r(location, ";", &last);
	while (last_location[index++] != NULL) {
		if (index == MAX_NPS_LOC_ITEM)
			break;
		last_location[index] = (char *)strtok_r(NULL, ";", &last);
	}
	index = 0;

	lbs_server_nps->last_pos.timestamp = timestamp;
	lbs_server_nps->last_pos.latitude = strtod(last_location[index++], NULL);
	lbs_server_nps->last_pos.longitude = strtod(last_location[index++], NULL);
	lbs_server_nps->last_pos.altitude = strtod(last_location[index++], NULL);
	lbs_server_nps->last_pos.speed = strtod(last_location[index++], NULL);
	lbs_server_nps->last_pos.direction = strtod(last_location[index++], NULL);
	lbs_server_nps->last_pos.hor_accuracy = strtod(last_location[index], NULL);

	LOG_NPS(DBG_LOW, "get nps_last_position timestamp : %d", lbs_server_nps->last_pos.timestamp);
}

static void nps_init(lbs_server_s *lbs_server_nps)
{
	LOG_NPS(DBG_LOW, "nps_init");

	lbs_server_nps->handle = NULL;
	lbs_server_nps->period = 5000;
	nps_set_status(lbs_server_nps, LBS_STATUS_UNAVAILABLE);

	lbs_server_nps->pos.fields = LBS_POSITION_EXT_FIELDS_NONE;
	lbs_server_nps->pos.timestamp = 0;
	lbs_server_nps->pos.latitude = 0.0;
	lbs_server_nps->pos.longitude = 0.0;
	lbs_server_nps->pos.altitude = 0.0;
	lbs_server_nps->pos.acc_level = LBS_POSITION_EXT_FIELDS_NONE;
	lbs_server_nps->pos.hor_accuracy = -1.0;
	lbs_server_nps->pos.ver_accuracy = -1.0;

	lbs_server_nps->last_pos.timestamp = 0;
	lbs_server_nps->last_pos.latitude = 0.0;
	lbs_server_nps->last_pos.longitude = 0.0;
	lbs_server_nps->last_pos.altitude = 0.0;
	lbs_server_nps->last_pos.hor_accuracy = 0.0;
	lbs_server_nps->last_pos.speed = 0.0;
	lbs_server_nps->last_pos.direction = 0.0;

#if !GLIB_CHECK_VERSION (2, 31, 0)
	GMutex *mutex_temp = g_mutex_new();
	lbs_server_nps->mutex = *mutex_temp;
	GCond *cond_temp = g_cond_new();
	lbs_server_nps->cond = *cond_temp;
#endif

	g_mutex_init(&lbs_server_nps->mutex);
	g_cond_init(&lbs_server_nps->cond);

	lbs_server_nps->is_nps_running = FALSE;
	lbs_server_nps->nps_client_count = 0;

	if (!get_nps_plugin_module()->load()) {
		LOG_NPS(DBG_ERR, "lbs_server_nps plugin load() failed.");
		return ;
	}

	nps_get_last_position(lbs_server_nps);
}

static void nps_deinit(lbs_server_s *lbs_server_nps)
{
	LOG_NPS(DBG_LOW, "nps_deinit");
	if (get_nps_plugin_module()->unload()) {
		if (lbs_server_nps->is_nps_running) {
			g_mutex_lock(&lbs_server_nps->mutex);
			lbs_server_nps->is_nps_running = FALSE;
			g_cond_signal(&lbs_server_nps->cond);
			g_mutex_unlock(&lbs_server_nps->mutex);
		}

		if (lbs_server_nps->token) {
			g_mutex_lock(&lbs_server_nps->mutex);
			g_free(lbs_server_nps->token);
			g_mutex_unlock(&lbs_server_nps->mutex);
		}
	} else {
		LOG_NPS(DBG_ERR, "unload() failed.");
	}

	lbs_server_nps->handle = NULL;
	lbs_server_nps->is_nps_running = FALSE;
	lbs_server_nps->nps_client_count = 0;

#if !GLIB_CHECK_VERSION (2, 31, 0)
	g_cond_free(&lbs_server_nps->cond);
	g_mutex_free(&lbs_server_nps->mutex);
#endif

	g_cond_clear(&lbs_server_nps->cond);
	g_mutex_clear(&lbs_server_nps->mutex);

	nps_set_status(lbs_server_nps, LBS_STATUS_UNAVAILABLE);
}

static void _glib_log(const gchar *log_domain, GLogLevelFlags log_level,
					const gchar *msg, gpointer user_data)
{
	LOG_NPS(DBG_ERR, "GLIB[%d] : %s", log_level, msg);
}

int main(int argc, char **argv)
{
	lbs_server_s *lbs_server = NULL;
	struct gps_callbacks cb;
	int ret_code = 0;
	cb.pos_cb = gps_update_position_cb;
	cb.batch_cb = gps_update_batch_cb;
	cb.sv_cb = gps_update_satellite_cb;
	cb.nmea_cb = gps_update_nmea_cb;

#if !GLIB_CHECK_VERSION (2, 31, 0)
	if (!g_thread_supported()) {
		g_thread_init(NULL);
	}
#endif

#if !GLIB_CHECK_VERSION (2, 35, 0)
	g_type_init();
#endif

	ret_code = initialize_server(argc, argv);
	if (ret_code != 0) {
		LOG_GPS(DBG_ERR, "initialize_server failed");
		return 1;
	}

	lbs_server = g_new0(lbs_server_s, 1);
	if (!lbs_server) {
		LOG_GPS(DBG_ERR, "lbs_server_s create fail");
		return 1;
	}
	lbs_server_init(lbs_server);
	gps_init_log();

	register_update_callbacks(&cb, lbs_server);

	g_log_set_default_handler(_glib_log, lbs_server);

	/* create lbs-dbus server */
	ret_code = lbs_server_create(SERVICE_NAME, SERVICE_PATH, "lbs-server", "lbs-server",
								&(lbs_server->lbs_dbus_server), set_options, shutdown, update_pos_tracking_interval,
								request_change_pos_update_interval, get_nmea,
								add_fence, remove_fence, pause_fence, resume_fence, (gpointer)lbs_server);
	if (ret_code != LBS_SERVER_ERROR_NONE) {
		LOG_GPS(DBG_ERR, "lbs_server_create failed");
		return 1;
	}

	LOG_GPS(DBG_LOW, "lbs_server_create called");

	lbs_server->loop = g_main_loop_new(NULL, TRUE);
	g_main_loop_run(lbs_server->loop);

	LOG_GPS(DBG_LOW, "lbs_server deamon Stop....");

	gps_deinit_log();

	/* destroy resource for dynamic-interval */
	g_free(lbs_server->optimized_interval_array);
	g_hash_table_destroy(lbs_server->dynamic_interval_table);

	/* destroy lbs-dbus server */
	lbs_server_destroy(lbs_server->lbs_dbus_server);
	LOG_GPS(DBG_LOW, "lbs_server_destroy called");

	g_main_loop_unref(lbs_server->loop);

	nps_deinit(lbs_server);
	g_free(lbs_server);

	deinitialize_server();

	return 0;
}
