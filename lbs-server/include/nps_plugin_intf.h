/*
 * lbs-server
 *
 * Copyright (c) 2010-2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact: Youngae Kang <youngae.kang@samsung.com>,
 *			Genie Kim <daejins.kim@samsung.com>, Minjune Kim <sena06.kim@samsung.com>,
 *			Ming Zhu <mingwu.zhu@samsung.com>
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

#ifndef _NPS_PLUGIN_INTF_H_
#define _NPS_PLUGIN_INTF_H_

#define NPS_UNIQUE_ID_LEN (20)
#define NPS_PLUGIN_PATH	"/usr/lib/libSLP-nps-plugin.so"

typedef struct {
	double latitude;	/* decimal degree (WGS84) */
	double longitude;	/* decimal degree (WGS84) */
	double hpe;			/* in meters (@ 95% confidence) */
	double altitude;	/* in meters */
	double speed;		/* in km/hr (negative value for unknown speed) */
	double bearing;		/* degree from north clockwise (+90 is East) (negative value for unknown) */
	unsigned long age;	/* number of milliseconds elapsed since location was calculated */
} Plugin_LocationInfo;


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*LocationCallback)(void *arg, const Plugin_LocationInfo *location, const void *reserved);
typedef void (*OfflineTokenCallback)(void *arg, const unsigned char *token, unsigned tokenSize);
typedef void (*CancelCallback)(void *arg);

typedef struct {
	int (*load)(void);
	int (*unload)(void);
	int (*start)(unsigned long period, LocationCallback cb, void *arg, void **handle);
	int (*stop)(void *handle, CancelCallback cb, void *arg);
	void (*getOfflineToken)(const unsigned char *key, unsigned int keyLengh, OfflineTokenCallback cb, void *arg);
	int (*offlineLocation)(const unsigned char *key, unsigned int keyLength, const unsigned char *token, unsigned int tokenSize, LocationCallback cb, void *arg);
	void (*cellLocation)(LocationCallback callback, void *arg);
} nps_plugin_interface;

const nps_plugin_interface *get_nps_plugin_interface();

#ifdef __cplusplus
}
#endif
#endif /* _NPS_PLUGIN_INTF_H_ */

