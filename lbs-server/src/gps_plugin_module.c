/*
 * lbs_server
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
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "gps_plugin_module.h"
#include "setting.h"
#include "debug_util.h"

#define SPECIFIC_PLUGIN_PATH_PREFIX	LIB_DIR"/liblbs-server-plugin-"
#define SPECIFIC_PLUGIN_PATH_POSTFIX	".so"

static const gps_plugin_interface *g_plugin = NULL;

int load_plugin_module(char *specific_name, void **plugin_handle)
{
	char plugin_path[256];

	if (specific_name[0] == '\0') {
		strncpy(plugin_path, GPS_PLUGIN_PATH, sizeof(plugin_path));
	} else {
		snprintf(plugin_path, sizeof(plugin_path),
				SPECIFIC_PLUGIN_PATH_PREFIX"%s"
				SPECIFIC_PLUGIN_PATH_POSTFIX,
				specific_name);

		struct stat st = {0};

		if (stat(plugin_path, &st) != 0) {
			strncpy(plugin_path, GPS_PLUGIN_PATH, sizeof(plugin_path));
			/* To support real GPS when additional plugin is added*/
			/* setting_set_int(VCONFKEY_LOCATION_REPLAY_ENABLED, 1); */
		}
	}

	*plugin_handle = dlopen(plugin_path, RTLD_NOW);
	if (!*plugin_handle) {
		LOG_GPS(DBG_ERR, "Failed to load plugin module: %s", plugin_path);
		/* LOG_GPS(DBG_ERR, "%s", dlerror()); */
		return FALSE;
	}

	const gps_plugin_interface *(*get_gps_plugin_interface)();
	get_gps_plugin_interface = dlsym(*plugin_handle, "get_gps_plugin_interface");
	if (!get_gps_plugin_interface) {
		LOG_GPS(DBG_ERR, "Failed to find entry symbol in plugin module.");
		dlclose(*plugin_handle);
		return FALSE;
	}

	g_plugin = get_gps_plugin_interface();

	if (!g_plugin) {
		LOG_GPS(DBG_ERR, "Failed to find load symbol in plugin module.");
		dlclose(*plugin_handle);
		return FALSE;
	}
	LOG_GPS(DBG_LOW, "Success to load plugin module");

	return TRUE;
}

int unload_plugin_module(void *plugin_handle)
{
	if (plugin_handle == NULL) {
		LOG_GPS(DBG_ERR, "plugin_handle is already NULL.");
		return FALSE;
	}

	dlclose(plugin_handle);

	return TRUE;
}

const gps_plugin_interface *get_plugin_module(void)
{
	return g_plugin;
}
