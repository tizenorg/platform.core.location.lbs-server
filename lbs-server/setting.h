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

#ifndef _SETTING_H_
#define _SETTING_H_

#include <vconf.h>
#include <vconf-internal-location-keys.h>

#include <glib.h>

#define NPS_UNIQUE_ID_LEN (20)

//#define NPS_MANAGER_PLUGIN_PATH	"/usr/lib/libSLP-nps-plugin.so"

typedef enum {
	POSITION_OFF = 0,
	POSITION_SEARCHING,
	POSITION_CONNECTED,
	POSITION_MAX
} pos_state_t;

int setting_set_int(const char *path, int val);
int setting_get_int(const char *path, int *val);
int setting_set_double(const char *path, double val);
int setting_get_double(const char *path, double *val);
int setting_set_string(const char *path, const char *val);
char *setting_get_string(const char *path);

typedef void (*key_changed_cb) (keynode_t * key, void *data);

int setting_notify_key_changed(const char *path, void *key_changed_cb, void *data);
int setting_ignore_key_changed(const char *path, void *key_changed_cb);

unsigned char setting_get_unique_id(char *unique_id);

#endif				/* _SETTING_H_ */
