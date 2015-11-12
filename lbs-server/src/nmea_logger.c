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
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "nmea_logger.h"
#include "debug_util.h"

#define MAX_NMEA_RAW_DATA_LOG_FILE_CNT	(999)
#define MAX_NMEA_LOG_FILE_PATH		(100)

#define PHONE_FOLDER			"/opt/usr/media"
#define GPS_FOLDER		PHONE_FOLDER"/lbs-server"
#define NMEA_FOLDER			GPS_FOLDER"/NMEA"
#define NMEA_LOGGING_FILE_PATH		NMEA_FOLDER"/nmea_data"

int raw_nmea_fd = -1;

static int generate_nmea_log_file(char *);

void start_nmea_log()
{
	char filepath[MAX_NMEA_LOG_FILE_PATH];

	/* File Open */
	struct stat st = {0};

	if (stat(GPS_FOLDER, &st) == -1) {
		if (mkdir(GPS_FOLDER, 0777) == -1) {
			LOG_GPS(DBG_ERR, "Fail to make lbs-server folder");
			raw_nmea_fd = -1;
			return;
		} else {
			if (mkdir(NMEA_FOLDER, 0777) == -1) {
				LOG_GPS(DBG_ERR, "Fail to make NMEA folder");
				raw_nmea_fd = -1;
				return;
			}
		}
	} else {
		if (stat(NMEA_FOLDER, &st) == -1) {
			if (mkdir(NMEA_FOLDER, 0777) == -1) {
				LOG_GPS(DBG_ERR, "Fail to make NMEA folder");
				raw_nmea_fd = -1;
				return;
			}
		}
	}

	if (generate_nmea_log_file(filepath) == -1) {
		LOG_GPS(DBG_ERR, "Starting LBS Logging for RAW NMEA data FAILED!");
		raw_nmea_fd = -1;
		return;
	}

	raw_nmea_fd = open(filepath, O_RDWR | O_APPEND | O_CREAT, 0644);

	if (raw_nmea_fd < 0) {
		LOG_GPS(DBG_ERR, "FAILED to open nmea_data file, error[%d]", errno);
	} else {
		LOG_GPS(DBG_LOW, "Success :: raw_nmea_fd [%d]", raw_nmea_fd);
	}

	return;
}

void stop_nmea_log()
{
	int close_ret_val = 0;

	LOG_GPS(DBG_LOW, "raw_nmea_fd [%d]", raw_nmea_fd);

	if (raw_nmea_fd != -1) {
		close_ret_val = close(raw_nmea_fd);
		if (close_ret_val < 0) {
			LOG_GPS(DBG_ERR, "FAILED to close raw_nmea_fd[%d], error[%d]", raw_nmea_fd, errno);
		}
		raw_nmea_fd = -1;
	}
	return;
}

void write_nmea_log(char *data, int len)
{
	int write_ret_val = 0;

	if (raw_nmea_fd != -1) {
		write_ret_val = write(raw_nmea_fd, data, len);
		if (write_ret_val < 0) {
			LOG_GPS(DBG_ERR, "FAILED to write[%d], error[%d]", raw_nmea_fd, errno);
			close(raw_nmea_fd);
			raw_nmea_fd = -1;
		}
	}
	return;
}

static int generate_nmea_log_file(char *filepath)
{
	int idx = 0;
	int fd = 0;
	char fn[MAX_NMEA_LOG_FILE_PATH] = {0,};

	for (idx = 0; idx < MAX_NMEA_RAW_DATA_LOG_FILE_CNT; idx++) {
		g_snprintf(fn, MAX_NMEA_LOG_FILE_PATH, "%s%03d.txt", NMEA_LOGGING_FILE_PATH, idx);
		if ((fd = access(fn, R_OK)) == -1) {
			LOG_GPS(DBG_LOW, "Next log file [%s]", fn);
			g_strlcpy(filepath, fn, strlen(fn) + 1);
			return 0;
		}
	}
	LOG_GPS(DBG_LOW, "All NMEA RAW Data logging files are used. New log file can not be created");
	return -1;
}
