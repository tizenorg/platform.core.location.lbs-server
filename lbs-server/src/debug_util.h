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

#ifndef _DEBUG_UTIL_H_
#define _DEBUG_UTIL_H_

#include <glib.h>
#include <libgen.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <dlog.h>
#define TAG_GPS_MANAGER		"LBS_SERVER_GPS"
#define TAG_NPS_MANAGER		"LBS_SERVER_NPS"

#define DBG_LOW		LOG_DEBUG
#define DBG_INFO	LOG_INFO
#define DBG_WARN	LOG_WARN
#define DBG_ERR		LOG_ERROR

#define LOG_GPS(dbg_lvl,fmt,args...)  SLOG(dbg_lvl, TAG_GPS_MANAGER, fmt, ##args)
#define LOG_NPS(dbg_lvl,fmt,args...)  SLOG(dbg_lvl, TAG_NPS_MANAGER, fmt,##args)
#define FUNC_ENTRANCE_SERVER		LOG_GPS(DBG_LOW, "[%s] Entered!!", __func__);

#ifdef __cplusplus
}
#endif
#endif				/* _DEBUG_UTIL_H_ */
