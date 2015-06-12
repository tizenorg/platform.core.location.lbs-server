/*
 * lbs-server
 *
 * Copyright (c) 2010-2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact: Youngae Kang <youngae.kang@samsung.com>, Dong Wei <d.wei@samsung.com>,
 *          Genie Kim <daejins.kim@samsung.com>, Minjune Kim <sena06.kim@samsung.com>,
 *          Ming Zhu <mingwu.zhu@samsung.com>, Congyi Shi <congyi.shi@samsung.com>
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
#include "nps_plugin_module.h"
#include "setting.h"
#include "debug_util.h"

#define PLUGIN_PATH_POSTFIX	".so"


static const nps_plugin_interface  *g_plugin = NULL;
static int g_is_nps_dummy_module = FALSE;

int dummy_load(void);
int dummy_unload(void);
int dummy_start(unsigned long period, LocationCallback cb, void *arg, void **handle);
int dummy_stop(void *handle, CancelCallback cb, void *arg);
void dummy_get_offline_token(const unsigned char *key, unsigned int keyLengh, OfflineTokenCallback cb, void *arg);
int dummy_offline_location(const unsigned char *key, unsigned int keyLength, const unsigned char *token, unsigned int tokenSize, LocationCallback cb, void *arg);
void dummy_cell_location(LocationCallback callback, void *arg);

static nps_plugin_interface g_dummy = {
	.load = dummy_load,
	.unload = dummy_unload,
	.start = dummy_start,
	.stop = dummy_stop,
	.getOfflineToken = dummy_get_offline_token,
	.offlineLocation = dummy_offline_location,
	.cellLocation = dummy_cell_location
};

int nps_is_dummy_plugin_module()
{
	return g_is_nps_dummy_module;
}

int nps_load_plugin_module(void **plugin_handle)
{
	LOG_NPS(DBG_LOW, "Begin to load plugin module");

	char plugin_path[256] = {0};

	strncpy(plugin_path, NPS_PLUGIN_PATH, sizeof(plugin_path));

	if (access(plugin_path, R_OK) != 0) {
		LOG_NPS(DBG_ERR, "Failed to access plugin module. [%s]", plugin_path);
		LOG_NPS(DBG_LOW, "load dummy");
		g_plugin = &g_dummy;
		g_is_nps_dummy_module = TRUE;
		return TRUE;
	}

	*plugin_handle = dlopen(plugin_path, RTLD_NOW);
	if (!*plugin_handle) {
		LOG_NPS(DBG_ERR, "Failed to load plugin module. [%s]", plugin_path);
		/* LOG_NPS(DBG_ERR, "%s", dlerror()); */
		return FALSE;
	}

	const nps_plugin_interface *(*get_nps_plugin_interface)();
	get_nps_plugin_interface = dlsym(*plugin_handle, "get_nps_plugin_interface");
	if (!get_nps_plugin_interface) {
		LOG_NPS(DBG_ERR, "Failed to find entry symbol in plugin module.");
		dlclose(*plugin_handle);
		return FALSE;
	}

	g_plugin = get_nps_plugin_interface();

	if (!g_plugin) {
		LOG_NPS(DBG_ERR, "Failed to find load symbol in plugin module.");
		dlclose(*plugin_handle);
		return FALSE;
	}
	LOG_NPS(DBG_LOW, "Success to load plugin module (%s).", plugin_path);

	return TRUE;
}

int nps_unload_plugin_module(void *plugin_handle)
{
	if (plugin_handle == NULL) {
		LOG_NPS(DBG_ERR, "plugin_handle is already NULL.");
		return FALSE;
	}

	dlclose(plugin_handle);

	if (g_plugin) {
		g_plugin = NULL;
	}
	return TRUE;
}

const nps_plugin_interface *get_nps_plugin_module(void)
{
	return g_plugin;
}

int dummy_load(void)
{
	LOG_NPS(DBG_ERR, "Dummy func.");
	return FALSE;
}

int dummy_unload(void)
{
	LOG_NPS(DBG_ERR, "Dummy func.");
	return FALSE;
}

int dummy_start(unsigned long period, LocationCallback cb, void *arg, void **handle)
{
	LOG_NPS(DBG_ERR, "Dummy func.");
	return FALSE;
}

int dummy_stop(void *handle, CancelCallback cb, void *arg)
{
	LOG_NPS(DBG_ERR, "Dummy func.");
	return FALSE;
}

void dummy_get_offline_token(const unsigned char *key, unsigned int keyLengh, OfflineTokenCallback cb, void *arg)
{
	LOG_NPS(DBG_ERR, "Dummy func.");
}

int dummy_offline_location(const unsigned char *key, unsigned int keyLength, const unsigned char *token, unsigned int tokenSize, LocationCallback cb, void *arg)
{
	LOG_NPS(DBG_ERR, "Dummy func.");
	return FALSE;
}

void dummy_cell_location(LocationCallback callback, void *arg)
{
	LOG_NPS(DBG_ERR, "Dummy func.");
}
