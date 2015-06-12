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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <glib.h>

#include "debug_util.h"

#define GPG_DUMP_LOG	"/tmp/dump_gps.log"

int fd = -1;

struct tm *__get_current_time() {
	time_t now;
	struct tm *cur_time;

	time(&now);
	cur_time = localtime(&now);
	return cur_time;
}

void gps_init_log()
{
	LOG_GPS(DBG_ERR, "gps_init_log");
	int ret = -1;
	struct tm *cur_time = NULL;
	char buf[256] = {0, };
	fd = open(GPG_DUMP_LOG, O_RDWR | O_APPEND | O_CREAT, 0644);
	if (fd < 0) {
		LOG_GPS(DBG_ERR, "Fail to open file[%s]", GPG_DUMP_LOG);
		return;
	}

	cur_time = __get_current_time();
	if (!cur_time) {
		LOG_GPS(DBG_ERR, "Can't get current time[%s]", GPG_DUMP_LOG);
		return;
	}

	g_snprintf(buf, 256, "[%02d:%02d:%02d] -- START GPS -- \n", cur_time->tm_hour, cur_time->tm_min, cur_time->tm_sec);
	ret = write(fd, buf, strlen(buf));
	if (ret == -1) {
		LOG_GPS(DBG_ERR, "Fail to write file[%s]", GPG_DUMP_LOG);
	}
}

void gps_deinit_log()
{
	LOG_GPS(DBG_ERR, "gps_deinit_log");
	if (fd < 0) return;
	int ret = -1;
	struct tm *cur_time = __get_current_time();
	char buf[256] = {0, };

	if (!cur_time) {
		LOG_GPS(DBG_ERR, "Can't get current time[%s]", GPG_DUMP_LOG);
	} else {
		g_snprintf(buf, 256, "[%02d:%02d:%02d] -- END GPS -- \n", cur_time->tm_hour, cur_time->tm_min, cur_time->tm_sec);
		ret = write(fd, buf, strlen(buf));
		if (ret == -1) {
			LOG_GPS(DBG_ERR, "Fail to write file[%s]", GPG_DUMP_LOG);
		}
	}
	close(fd);
	fd = -1;
}

void gps_dump_log(const char *str, const char *app_id)
{
	if (fd < 0) {
		LOG_GPS(DBG_ERR, "Not available fd[%d]", fd);
		return;
	}
	int ret = -1;
	char buf[256] = {0, };
	struct tm *cur_time = __get_current_time();

	if (!cur_time) {
		LOG_GPS(DBG_ERR, "Can't get current time[%s]", GPG_DUMP_LOG);
		return;
	}

	if (app_id == NULL) {
		g_snprintf(buf, 256, "[%02d:%02d:%02d] %s\n", cur_time->tm_hour, cur_time->tm_min, cur_time->tm_sec, str);
	} else {
		g_snprintf(buf, 256, "[%02d:%02d:%02d] %s from [%s]\n", cur_time->tm_hour, cur_time->tm_min, cur_time->tm_sec, str, app_id);
	}
	LOG_GPS(DBG_ERR, "Add dump log [%s", buf);
	ret = write(fd, buf, strlen(buf));
	if (ret == -1) {
		LOG_GPS(DBG_ERR, "Fail to write file[%s]", GPG_DUMP_LOG);
	}
}
