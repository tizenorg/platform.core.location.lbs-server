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
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>
#include <setjmp.h>

#include "data_connection.h"
#include <network-cm-intf.h>
#include "debug_util.h"

int noti_resp_init(int *noti_pipe_fds);
int noti_resp_wait(int *noti_pipe_fds);
int noti_resp_check(int *noti_pipe_fds);
int noti_resp_noti(int *noti_pipe_fds, int result);
int noti_resp_deinit(int *noti_pipe_fds);

unsigned int g_ipaddr;
static int pdp_pipe_fds[2];

typedef enum {
	DEACTIVATED,
	ACTIVATING,
	ACTIVATED,
	DEACTIVATING
} pdp_status;

char profile_name[NET_PROFILE_NAME_LEN_MAX + 1];

pdp_status act_level;

char *PdpStat[] = { "Deactivated", "Activating", "Activated", "Deactivating" };

static void set_connection_status(pdp_status i)
{
	act_level = i;
	LOG_GPS(DBG_LOW, "==Status: %s\n", PdpStat[i]);
}

static pdp_status get_connection_status()
{
	return act_level;
}

static void pdp_proxy_conf()
{
	net_proxy_t proxy;
	int ret;
	ret = net_get_active_proxy(&proxy);

	if (ret == NET_ERR_NONE) {
		if (strncmp(proxy.proxy_addr, "0.0.0.0", 7)) {
			char buf[100];
			snprintf(buf, sizeof(buf), "http://%s/", proxy.proxy_addr);
			setenv("http_proxy", buf, 1);
		} else {
			unsetenv("http_proxy");
		}
	} else {
		LOG_GPS(DBG_ERR, "Fail to get proxy\n");
	}
}

void pdp_evt_cb(net_event_info_t *event_cb, void *user_data)
{
	switch (event_cb->Event) {
		case NET_EVENT_OPEN_RSP: {
				LOG_GPS(DBG_LOW, "event_cb->Error : [%d]\n", event_cb->Error);
				if (get_connection_status() != ACTIVATING) {
					LOG_GPS(DBG_LOW, "Not Activating status\n");
				} else if (event_cb->Error == NET_ERR_NONE || event_cb->Error == NET_ERR_ACTIVE_CONNECTION_EXISTS) {
					LOG_GPS(DBG_LOW, "Successful PDP Activation\n");
					net_profile_info_t *prof_info = NULL;
					net_dev_info_t *net_info = NULL;

					prof_info = (net_profile_info_t *) event_cb->Data;
					net_info = &(prof_info->ProfileInfo.Pdp.net_info);

					strncpy(profile_name, net_info->ProfileName, NET_PROFILE_NAME_LEN_MAX);

					set_connection_status(ACTIVATED);
					pdp_proxy_conf();
					noti_resp_noti(pdp_pipe_fds, TRUE);
				} else {
					LOG_GPS(DBG_ERR, " PDP Activation Failed - PDP Error[%d]\n", event_cb->Error);
					set_connection_status(DEACTIVATED);
					noti_resp_noti(pdp_pipe_fds, FALSE);
				}
			}
			break;

		case NET_EVENT_CLOSE_RSP:
			if (get_connection_status() != ACTIVATED && get_connection_status() != DEACTIVATING) {
				LOG_GPS(DBG_ERR, "Not Activated && Deactivating status\n");
			} else if (event_cb->Error == NET_ERR_NONE || event_cb->Error == NET_ERR_UNKNOWN) {
				LOG_GPS(DBG_LOW, "Successful PDP De-Activation\n");
				set_connection_status(DEACTIVATED);
				noti_resp_noti(pdp_pipe_fds, TRUE);
			} else {
				LOG_GPS(DBG_ERR, " PDP DeActivation Failed - PDP Error[%d]\n", event_cb->Error);
				noti_resp_noti(pdp_pipe_fds, FALSE);
			}
			break;

		case NET_EVENT_CLOSE_IND:
			if (get_connection_status() != ACTIVATED && get_connection_status() != DEACTIVATING) {
				LOG_GPS(DBG_ERR, "Not Activated && Deactivating status\n");
			} else if (event_cb->Error == NET_ERR_NONE || event_cb->Error == NET_ERR_UNKNOWN) {
				LOG_GPS(DBG_LOW, "Successful PDP De-Activation\n");
				set_connection_status(DEACTIVATED);
				noti_resp_noti(pdp_pipe_fds, TRUE);
			} else {
				LOG_GPS(DBG_ERR, " PDP DeActivation Failed - PDP Error[%d]\n", event_cb->Error);
				noti_resp_noti(pdp_pipe_fds, FALSE);
			}
			break;
		case NET_EVENT_OPEN_IND:
			break;
		default:
			break;
	}
}

unsigned int start_pdp_connection(void)
{
	int err = -1;

	set_connection_status(DEACTIVATED);
	if (noti_resp_init(pdp_pipe_fds)) {
		LOG_GPS(DBG_LOW, "Success start_pdp_connection\n");
	} else {
		LOG_GPS(DBG_ERR, "ERROR in noti_resp_init()\n");
		return FALSE;
	}

	err = net_register_client((net_event_cb_t) pdp_evt_cb, NULL);

	if (err == NET_ERR_NONE || err == NET_ERR_APP_ALREADY_REGISTERED) {
		LOG_GPS(DBG_LOW, "Client registration is succeed\n");
	} else {
		LOG_GPS(DBG_WARN, "Error in net_register_client [%d]\n", err);
		noti_resp_deinit(pdp_pipe_fds);
		return FALSE;
	}

	set_connection_status(ACTIVATING);
	err = net_open_connection_with_preference(NET_SERVICE_INTERNET);

	if (err == NET_ERR_NONE) {
		LOG_GPS(DBG_LOW, "Sent PDP Activation.\n");
	} else {
		LOG_GPS(DBG_WARN, "Error in net_open_connection_with_preference() [%d]\n", err);
		set_connection_status(DEACTIVATED);
		err = net_deregister_client();
		if (err == NET_ERR_NONE) {
			LOG_GPS(DBG_LOW, "Client deregistered successfully\n");
		} else {
			LOG_GPS(DBG_ERR, "Error deregistering the client\n");
		}
		noti_resp_deinit(pdp_pipe_fds);
		return FALSE;
	}

	if (noti_resp_wait(pdp_pipe_fds) > 0) {
		if (noti_resp_check(pdp_pipe_fds)) {
			LOG_GPS(DBG_LOW, "PDP Activation Successful\n");
			noti_resp_deinit(pdp_pipe_fds);
			return TRUE;
		} else {
			LOG_GPS(DBG_ERR, "PDP failed\n");
			noti_resp_deinit(pdp_pipe_fds);

			err = net_deregister_client();
			if (err == NET_ERR_NONE) {
				LOG_GPS(DBG_LOW, "Client deregistered successfully\n");
			} else {
				LOG_GPS(DBG_ERR, "Error deregistering the client\n");
			}
			return FALSE;
		}
	} else {
		LOG_GPS(DBG_ERR, "NO Pdp Act Response or Some error in select.\n");
		noti_resp_deinit(pdp_pipe_fds);

		err = net_deregister_client();
		if (err == NET_ERR_NONE) {
			LOG_GPS(DBG_LOW, "Client deregistered successfully\n");
		} else {
			LOG_GPS(DBG_ERR, "Error deregistering the client\n");
		}
		return FALSE;
	}
}

unsigned int stop_pdp_connection(void)
{
	int err;
	pdp_status pStatus = get_connection_status();
	if (pStatus == DEACTIVATED || pStatus == DEACTIVATING) {
		LOG_GPS(DBG_ERR, "pdp stop progressing already. pStatus[%d] \n", pStatus);
		return TRUE;
	}

	if (noti_resp_init(pdp_pipe_fds)) {
		LOG_GPS(DBG_LOW, "Success stop_pdp_connection\n");
	} else {
		LOG_GPS(DBG_ERR, "ERROR in noti_resp_init()\n");
		return FALSE;
	}

	set_connection_status(DEACTIVATING);
	err = net_close_connection(profile_name);
	if (err == NET_ERR_NONE) {
		LOG_GPS(DBG_LOW, "Success net_close_connection\n");
	} else {
		LOG_GPS(DBG_WARN, "Error in sending net_close_connection error=%d\n", err);
		set_connection_status(pStatus);
		noti_resp_deinit(pdp_pipe_fds);
		return FALSE;
	}
	if (noti_resp_wait(pdp_pipe_fds) > 0) {
		if (noti_resp_check(pdp_pipe_fds)) {
			LOG_GPS(DBG_LOW, "Close Connection success\n");
		} else {
			LOG_GPS(DBG_ERR, "Close connection failed\n");
		}
	}

	noti_resp_deinit(pdp_pipe_fds);

	set_connection_status(DEACTIVATED);

	err = net_deregister_client();
	if (err == NET_ERR_NONE) {
		LOG_GPS(DBG_LOW, "Client deregistered successfully\n");
	} else {
		LOG_GPS(DBG_WARN, "Error deregistering the client\n");
		return FALSE;
	}

	return TRUE;
}

unsigned int query_dns(char *pdns_lookup_addr, unsigned int *ipaddr, int *port)
{
	FUNC_ENTRANCE_SERVER;
	/*int dns_id; */
	unsigned int ret = 0;
	struct hostent *he;

	char *colon = strchr(pdns_lookup_addr, ':');
	char *last = NULL;
	if (colon != NULL) {
		char *ptr = (char *)strtok_r(pdns_lookup_addr, ":", &last);
		pdns_lookup_addr = ptr;

		ptr = (char *)strtok_r(NULL, ":", &last);
		*port = atoi(ptr);
	}

	he = gethostbyname(pdns_lookup_addr);

	if (he != NULL) {
		LOG_GPS(DBG_LOW, "g_agps_ipaddr: %u\n", g_ipaddr);
		*ipaddr = g_ipaddr;

		ret = TRUE;
	} else {
		LOG_GPS(DBG_ERR, "//gethostbyname : Error getting host information\n");
		ret = FALSE;
	}

	return ret;

}

int noti_resp_init(int *noti_pipe_fds)
{
	if (pipe(noti_pipe_fds) < 0) {
		return 0;
	} else {
		return 1;
	}
}

int noti_resp_wait(int *noti_pipe_fds)
{
	fd_set rfds;
	fd_set wfds;
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_SET(*noti_pipe_fds, &rfds);
	return select(*noti_pipe_fds + 1, &rfds, &wfds, NULL, NULL);
}

int noti_resp_noti(int *noti_pipe_fds, int result)
{
	return write(*(noti_pipe_fds + 1), &result, sizeof(int));
}

int noti_resp_check(int *noti_pipe_fds)
{
	int activation = 0;
	ssize_t ret_val = 0;
	ret_val = read(*noti_pipe_fds, &activation, sizeof(int));
	if (ret_val == 0) {
		LOG_GPS(DBG_ERR, "No data");
	}
	return activation;
}

int noti_resp_deinit(int *noti_pipe_fds)
{
	int err;
	err = close(*noti_pipe_fds);
	if (err != 0) {
		LOG_GPS(DBG_ERR, "ERROR closing fds1.\n");
	}

	err = close(*(noti_pipe_fds + 1));
	if (err != 0) {
		LOG_GPS(DBG_ERR, "ERROR closing fds2.\n");
	}

	return err;
}
