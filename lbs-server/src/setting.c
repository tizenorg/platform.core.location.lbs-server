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

#include <string.h>
#include "setting.h"
#include "debug_util.h"

#include <stdlib.h>
#include <stdio.h>
#include <wifi.h>
#include <unistd.h>

int setting_set_int(const char *path, int val)
{
	int ret = vconf_set_int(path, val);
	if (ret == 0) {
		ret = TRUE;
	} else {
		LOG_GPS(DBG_ERR, "vconf_set_int failed, [%s]", path);
		ret = FALSE;
	}
	return ret;
}

int setting_get_int(const char *path, int *val)
{
	int ret = vconf_get_int(path, val);
	if (ret == 0) {
		ret = TRUE;
	} else {
		LOG_GPS(DBG_ERR, "vconf_get_int failed, [%s]", path);
		ret = FALSE;
	}
	return ret;
}

int setting_set_double(const char *path, double val)
{
	int ret = vconf_set_dbl(path, val);
	if (ret == 0) {
		ret = TRUE;
	} else {
		LOG_GPS(DBG_ERR, "vconf_set_dbl failed, [%s]", path);
		ret = FALSE;
	}
	return ret;
}

int setting_get_double(const char *path, double *val)
{
	int ret = vconf_get_dbl(path, val);
	if (ret == 0) {
		ret = TRUE;
	} else {
		LOG_GPS(DBG_ERR, "vconf_get_int failed, [%s]", path);
		ret = FALSE;
	}
	return ret;
}

int setting_set_string(const char *path, const char *val)
{
	int ret = vconf_set_str(path, val);
	if (ret == 0) {
		ret = TRUE;
	} else {
		LOG_GPS(DBG_ERR, "vconf_set_str failed, [%s]", path);
		ret = FALSE;
	}
	return ret;
}

char *setting_get_string(const char *path)
{
	return vconf_get_str(path);
}

int setting_notify_key_changed(const char *path, void *key_changed_cb, void *data)
{
	int ret = TRUE;
	if (vconf_notify_key_changed(path, key_changed_cb, data) != 0) {
		LOG_GPS(DBG_ERR, "Fail to vconf_notify_key_changed [%s]", path);
		ret = FALSE;
	}
	return ret;
}

int setting_ignore_key_changed(const char *path, void *key_changed_cb)
{
	int ret = TRUE;
	if (vconf_ignore_key_changed(path, key_changed_cb) != 0) {
		LOG_GPS(DBG_ERR, "Fail to vconf_ignore_key_changed [%s]", path);
		ret = FALSE;
	}
	return ret;
}


static unsigned char _get_mac_address(char *mac)
{
	int rv = 0;
	char *mac_addr = NULL;

	rv = wifi_get_mac_address(&mac_addr);
	if (rv != WIFI_ERROR_NONE)
		return FALSE;

	g_strlcpy(mac, mac_addr, NPS_UNIQUE_ID_LEN);
	g_free(mac_addr);

	return TRUE;
}

static unsigned char _get_device_name(char *device_name)
{
	char *ret_str;

	ret_str = vconf_get_str(VCONFKEY_SETAPPL_DEVICE_NAME_STR);
	if (ret_str == NULL) {
		return FALSE;
	}

	memcpy(device_name, ret_str, strlen(ret_str) + 1);

	return TRUE;
}

/*
Basic : Use BT address
BT Address : 12 digits (1234567890AB)
UNIQUEID : X X X 4 0 X 1 6 X 7 5 X 2 8 X A B 3 9
X: Model Name

Alternative 1: Use WiFi MAC Address
Cause : We can get local address and name only when bluetooth is on.
MAC Address : 17 digits
UNIQUEID : X X X 4 0 X 1 6 X 7 5 X 2 8 X A B 3 9
X: Device Name (We can't get the model name, setting is using system string.)
*/
unsigned char setting_get_unique_id(char *unique_id)
{
	char *mac_addr;
	char *device_name;

	mac_addr = g_malloc0(NPS_UNIQUE_ID_LEN);
	if (!_get_mac_address(mac_addr)) {
		g_free(mac_addr);
		return FALSE;
	}

	device_name = g_malloc0(NPS_UNIQUE_ID_LEN);
	if (!_get_device_name(device_name)) {
		g_free(mac_addr);
		g_free(device_name);
		return FALSE;
	}

	snprintf(unique_id, NPS_UNIQUE_ID_LEN, "%1.X%1.X%1.X%1.X%1.X%1.X%1.X%1.X%1.X%1.X%1.X%1.X%1.X%1.X%1.X%1.X%1.X%1.X%1.X",
			device_name[0] & 0xF, device_name[1] & 0xF, device_name[2] & 0xF, mac_addr[3] & 0xF,
			mac_addr[9] & 0xF, device_name[3] & 0xF, mac_addr[0] & 0xF, mac_addr[5] & 0xF,
			device_name[4] & 0xF, mac_addr[6] & 0xF, mac_addr[4] & 0xF, device_name[5] & 0xF,
			mac_addr[1] & 0xF, mac_addr[7] & 0xF, device_name[6] & 0xF, mac_addr[9] & 0xF,
			mac_addr[10] & 0xF, mac_addr[2] & 0xF, mac_addr[8] & 0xF);
	g_free(mac_addr);
	g_free(device_name);

	return TRUE;
}

