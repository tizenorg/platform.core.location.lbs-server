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

#ifndef _DATA_CONNECTION_H_
#define _DATA_CONNECTION_H_

/* Total Number of seconds in years between NTP Reference time(1900) to UTC reference time(1970)*/
#define UTC_NTP_BIAS 2208988800
/* Total Number of seconds in years between UTC reference time(1970) to GPS Refernce time(1980)*/
#define UTC_GPS_BIAS 315964800

#define MAX_NUMBER_OF_URLS 3

#include <glib.h>

unsigned int start_pdp_connection(void);
unsigned int stop_pdp_connection(void);

unsigned int query_dns(char *pdns_lookup_addr, unsigned int *ipaddr, int *port);

#endif
