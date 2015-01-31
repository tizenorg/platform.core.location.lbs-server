/*
 * gps-manager
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

#ifndef __MOD_LOG_H__
#define __MOD_LOG_H__

#define TAG_LOCATION		"LOCATION_GPS"

#include <dlog.h>
#define MOD_LOGD(fmt,args...)	SLOG(LOG_DEBUG,	TAG_LOCATION, fmt, ##args)
#define MOD_LOGW(fmt,args...)	SLOG(LOG_WARN,	TAG_LOCATION, fmt, ##args)
#define MOD_LOGI(fmt,args...)	SLOG(LOG_INFO,	TAG_LOCATION, fmt, ##args)
#define MOD_LOGE(fmt,args...)	SLOG(LOG_ERROR,	TAG_LOCATION, fmt, ##args)
#define MOD_SECLOG(fmt,args...)	SECURE_SLOG(LOG_DEBUG, TAG_LOCATION, fmt, ##args)

#define TAG_MOD_NPS		"LOCATION_NPS"

#include <dlog.h>
#define MOD_NPS_LOGD(fmt,args...)	SLOG(LOG_DEBUG,	TAG_MOD_NPS, fmt, ##args)
#define MOD_NPS_LOGW(fmt,args...)	SLOG(LOG_WARN,	TAG_MOD_NPS, fmt, ##args)
#define MOD_NPS_LOGI(fmt,args...)	SLOG(LOG_INFO,	TAG_MOD_NPS, fmt, ##args)
#define MOD_NPS_LOGE(fmt,args...)	SLOG(LOG_ERROR,	TAG_MOD_NPS, fmt, ##args)
#define MOD_NPS_SECLOG(fmt,args...)	SECURE_SLOG(LOG_DEBUG, TAG_MOD_NPS, fmt, ##args)


#endif
