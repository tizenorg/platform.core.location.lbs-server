/*
 * lbs-server
 *
 * Copyright (c) 2011-2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Youngae Kang <youngae.kang@samsung.com>, Minjune Kim <sena06.kim@samsung.com>
 *			Genie Kim <daejins.kim@samsung.com>
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

#ifndef _GPS_PLUGIN_PLUGIN_INTF_H_
#define _GPS_PLUGIN_PLUGIN_INTF_H_

#include <gps_plugin_data_types.h>
#include <gps_plugin_extra_data_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPS_PLUGIN_PATH	"/usr/lib/libSLP-lbs-plugin.so"
#define MAX_REQUESTER_ID_LEN	(128)
#define MAX_CLIENT_NAME_LEN	(128)
#define MAX_SUPL_URL_LEN	(128)

/**
 * GPS asynchronous event type
 */
typedef enum {
	GPS_EVENT_START_SESSION			= 0x0000,	/**< The session is started */
	GPS_EVENT_STOP_SESSION,						/**< The session is stopped */
	GPS_EVENT_CHANGE_INTERVAL,					/**< Change updating interval */
	GPS_EVENT_REPORT_POSITION		= 0x0100,	/**< Bring up GPS position data */
	GPS_EVENT_REPORT_SATELLITE,					/**< Bring up GPS SV data */
	GPS_EVENT_REPORT_NMEA,						/**< Bring up GPS NMEA data */
	GPS_EVENT_REPORT_BATCH,						/**< Bring up GPS batch data */
	GPS_EVENT_SET_OPTION			= 0x0200,	/**< The option is set */
	GPS_EVENT_GET_REF_LOCATION		= 0x0300,	/**< Get the reference location for AGPS */
	GPS_EVENT_GET_IMSI,							/**< Get IMSI for identification */
	GPS_EVENT_OPEN_DATA_CONNECTION	= 0x0400,	/**< Request opening data network connection */
	GPS_EVENT_CLOSE_DATA_CONNECTION,			/**< Request closing data network connection */
	GPS_EVENT_DNS_LOOKUP_IND,					/**< Request resolving host name */
	GPS_EVENT_AGPS_VERIFICATION_INDI,			/**< Verification indicator for AGPS is required */
	GPS_EVENT_FACTORY_TEST			= 0x0500,	/**< Factory test is done */
	GPS_EVENT_GEOFENCE_TRANSITION	= 0x0600,	/**< Geofence transition is occured */
	GPS_EVENT_GEOFENCE_STATUS,					/**< Report geofence serivce status */
	GPS_EVENT_ADD_GEOFENCE,						/**< Geofence is added(Start geofence) */
	GPS_EVENT_DELETE_GEOFENCE,					/**< Geofence is deleted(Stop geofence) */
	GPS_EVENT_PAUSE_GEOFENCE,					/**< Geofence is paused */
	GPS_EVENT_RESUME_GEOFENCE,					/**< Geofence is resumed */
	GPS_EVENT_ERR_CAUSE				= 0xFFFF	/**< Some error is occurred */
} gps_event_id_t;

/**
 * Start session response event data
 */
typedef struct {
	gps_error_t error;
} gps_start_session_ev_info_t;

/**
 * Response of stop session
 */
typedef struct {
	gps_error_t error;
} gps_stop_session_ev_info_t;

/**
 * Set option response event data
 */
typedef struct {
	gps_error_t error;
} gps_set_option_ev_info_t;

typedef struct {
	gps_error_t error;
} gps_change_interval_ev_info_t;

/**
 * Position data from GPS
 */
typedef struct {
	gps_error_t error;
	pos_data_t pos;
} gps_pos_ev_info_t;

/**
 * Batch data from GPS
 */
typedef struct {
	gps_error_t error;
	batch_data_t batch;
} gps_batch_ev_info_t;

/**
 * Satellite data from GPS
 */
typedef struct {
	gps_error_t error;
	sv_data_t sv;
} gps_sv_ev_info_t;

/**
 * NMEA data from GPS
 */
typedef struct {
	gps_error_t error;
	nmea_data_t nmea;
} gps_nmea_ev_info_t;

/**
 * This enumeration defines values for notify type for GPS verification message.
 */
typedef enum {
	AGPS_NOTIFY_NO_VERIFY			= 0x00,
	AGPS_NOTIFY_ONLY				= 0x01,
	AGPS_NOTIFY_ALLOW_NORESPONSE	= 0x02,
	AGPS_NOTIFY_NOTALLOW_NORESPONSE = 0x03,
	AGPS_NOTIFY_PRIVACY_NEEDED		= 0x04,
	AGPS_NOTIFY_PRIVACY_OVERRIDE	= 0x05
} agps_notify_t;

/**
 * This enumeration defines values for requester type for GPS verification message
 */
typedef enum {
	AGPS_REQ_LOGICAL_NAME	= 0x00,	/**< Specifies logical name. */
	AGPS_REQ_EMAIL_ADDR		= 0x01,	/**< Specifies e-mail address */
	AGPS_REQ_MSISDN			= 0x02,	/**< Specifies MSISDN number */
	AGPS_REQ_URL			= 0x03,	/**< Specifies URL */
	AGPS_REQ_SIPURL			= 0x04,	/**< Specifies SIPURL */
	AGPS_REQ_MIN			= 0x05,	/**< Specifies MIN */
	AGPS_REQ_MDN			= 0x06,	/**< Specifies MDN */
	AGPS_REQ_UNKNOWN		= 0x07	/**< Unknown request */
} agps_supl_format_t;

/**
 * This enumeration defines values for GPS encoding type for GPS verification message.
 */
typedef enum {
	AGPS_ENCODE_ISO646IRV		= 0x00,
	AGPS_ENCODE_ISO8859			= 0x01,
	AGPS_ENCODE_UTF8			= 0x02,
	AGPS_ENCODE_UTF16			= 0x03,
	AGPS_ENCODE_UCS2			= 0x04,
	AGPS_ENCODE_GSMDEFAULT		= 0x05,
	AGPS_ENCODE_SHIFT_JIS		= 0x06,
	AGPS_ENCODE_JIS				= 0x07,
	AGPS_ENCODE_EUC				= 0x08,
	AGPS_ENCODE_GB2312			= 0x09,
	AGPS_ENCODE_CNS11643		= 0x0A,
	AGPS_ENCODE_KSC1001			= 0x0B,
	AGPS_ENCODE_GERMAN			= 0x0C,
	AGPS_ENCODE_ENGLISH			= 0x0D,
	AGPS_ENCODE_ITALIAN			= 0x0E,
	AGPS_ENCODE_FRENCH			= 0x0F,
	AGPS_ENCODE_SPANISH			= 0x10,
	AGPS_ENCODE_DUTCH			= 0x11,
	AGPS_ENCODE_SWEDISH			= 0x12,
	AGPS_ENCODE_DANISH			= 0x13,
	AGPS_ENCODE_PORTUGUESE		= 0x14,
	AGPS_ENCODE_FINNISH			= 0x15,
	AGPS_ENCODE_NORWEGIAN		= 0x16,
	AGPS_ENCODE_GREEK			= 0x17,
	AGPS_ENCODE_TURKISH			= 0x18,
	AGPS_ENCODE_HUNGARIAN		= 0x19,
	AGPS_ENCODE_POLISH			= 0x1A,
	AGPS_ENCODE_LANGUAGE_UNSPEC	= 0xFF
} agps_encoding_scheme_t;

/**
 * This enumeration defines values for GPS encoding type for GPS verification message.
 */
typedef enum {
	AGPS_ID_ENCODE_ISO646IRV		= 0x00,
	AGPS_ID_ENCODE_EXN_PROTOCOL_MSG	= 0x01,
	AGPS_ID_ENCODE_ASCII			= 0x02,
	AGPS_ID_ENCODE_IA5				= 0x03,
	AGPS_ID_ENCODE_UNICODE			= 0x04,
	AGPS_ID_ENCODE_SHIFT_JIS		= 0x05,
	AGPS_ID_ENCODE_KOREAN			= 0x06,
	AGPS_ID_ENCODE_LATIN_HEBREW		= 0x07,
	AGPS_ID_ENCODE_LATIN			= 0x08,
	AGPS_ID_ENCODE_GSM				= 0x09
} agps_requester_id_encoding_t;

/**
 * This structure defines the values for GPS Verification message indication.
 */
typedef struct {
	/** Specifies notification type refer enum tapi_gps_notify_type_t */
	agps_notify_t				notify_type;

	/** Specifies encoding type refer enum tapi_gps_encoding_type_t */
	agps_supl_format_t			supl_format;

	/** Specifies requester type */
	agps_encoding_scheme_t		datacoding_scheme;

	agps_requester_id_encoding_t requester_id_encoding;

	/** Specifies requester ID */
	char						requester_id[MAX_REQUESTER_ID_LEN];

	/** Specifies client name */
	char						client_name[MAX_CLIENT_NAME_LEN];

	/** Response timer */
	int							resp_timer;
} agps_verification_ev_info_t;

/**
 * Factory test result information
 */
typedef struct {
	gps_error_t error;
	int			prn;
	double		snr;
} gps_factory_test_ev_info_t;

/**
 * DNS query request information
 */
typedef struct {
	gps_error_t error;
	char		domain_name[MAX_SUPL_URL_LEN];
} gps_dns_query_ev_info_t;

/**
 * Geofecne transition information
 */
typedef struct {
	time_t geofence_timestamp;
	int geofence_id;
	pos_data_t pos;
	geofence_zone_state_t state;
} geofence_transition_ev_info_t;

/**
 * Geofecne status information
 */
typedef struct {
	geofence_status_t status;
	pos_data_t last_pos;
} geofence_status_ev_info_t;

/**
 * Geofecne event information
 */
typedef struct {
	int geofence_id;
	geofence_error_t error;
} geofence_event_t;

/**
 * GPS event info
 */
typedef union {
	/** Callback related with Response */
	gps_start_session_ev_info_t		start_session_rsp;
	gps_stop_session_ev_info_t		stop_session_rsp;
	gps_set_option_ev_info_t		set_option_rsp;
	gps_change_interval_ev_info_t	change_interval_rsp;

	/** Callback related with Indication */
	gps_pos_ev_info_t			pos_ind;
	gps_sv_ev_info_t			sv_ind;
	gps_nmea_ev_info_t			nmea_ind;
	gps_batch_ev_info_t			batch_ind;
	agps_verification_ev_info_t agps_verification_ind;

	gps_factory_test_ev_info_t	factory_test_rsp;
	gps_dns_query_ev_info_t		dns_query_ind;

	/** Callback related with Geofence. */
	geofence_transition_ev_info_t	geofence_transition_ind;
	geofence_status_ev_info_t		geofence_status_ind;
	geofence_event_t				geofence_event_rsp;
} gps_event_data_t;

/**
 * Transport Error Cause
 */
typedef enum {
	GPS_FAILURE_CAUSE_NORMAL	= 0x00,
	GPS_FAILURE_CAUSE_FACTORY_TEST,
	GPS_FAILURE_CAUSE_DNS_QUERY
} gps_failure_reason_t;

/**
 * GPS Event Info
 */
typedef struct {
	gps_event_id_t		event_id;		/**< GPS asynchronous event id */
	gps_event_data_t	event_data;		/**< GPS event information data */
} gps_event_info_t;

/**
 * Callback function
 * LBS server needs to register a callback function with GPS OEM to receive asynchronous events.
 */
typedef int (*gps_event_cb)(gps_event_info_t *gps_event_info, void *user_data);

/**
 * GPS action type
 */
typedef enum {
	GPS_ACTION_SEND_PARAMS	= 0x000,
	GPS_ACTION_START_SESSION,
	GPS_ACTION_STOP_SESSION,
	GPS_ACTION_START_BATCH,
	GPS_ACTION_STOP_BATCH,
	GPS_ACTION_CHANGE_INTERVAL,

	GPS_INDI_SUPL_VERIFICATION,
	GPS_INDI_SUPL_DNSQUERY,

	GPS_ACTION_START_FACTTEST,
	GPS_ACTION_STOP_FACTTEST,
	GPS_ACTION_REQUEST_SUPL_NI,
	GPS_ACTION_DELETE_GPS_DATA,

	GPS_ACTION_ADD_GEOFENCE,
	GPS_ACTION_DELETE_GEOFENCE,
	GPS_ACTION_PAUSE_GEOFENCE,
	GPS_ACTION_RESUME_GEOFENCE,
} gps_action_t;

/**
 * Cell information type
 */
typedef enum {
	GPS_CELL_INFO_TYPE_aRFCNPresent = 0,
	GPS_CELL_INFO_TYPE_bSICPresent,
	GPS_CELL_INFO_TYPE_rxLevPresent,
	GPS_CELL_INFO_TYPE_frequencyInfoPresent,
	GPS_CELL_INFO_TYPE_cellMeasuredResultPresent,

	GPS_CELL_INFO_TYPE_refMCC,
	GPS_CELL_INFO_TYPE_refMNC,
	GPS_CELL_INFO_TYPE_refLAC,
	GPS_CELL_INFO_TYPE_refCI,
	GPS_CELL_INFO_TYPE_refUC,
	GPS_CELL_INFO_TYPE_aRFCN,
	GPS_CELL_INFO_TYPE_bSIC,
	GPS_CELL_INFO_TYPE_rxLev
} agps_cell_info_t;

/**
 * Mobile service type
 */
typedef enum {
	SVCTYPE_NONE = 0,	/**< Unknown network */
	SVCTYPE_NOSVC,		/**< Network in no service */
	SVCTYPE_EMERGENCY,	/**< Network emergency */
	SVCTYPE_SEARCH,		/**< Network search 1900 */
	SVCTYPE_2G,		/**< Network 2G */
	SVCTYPE_2_5G,		/**< Network 2.5G */
	SVCTYPE_2_5G_EDGE,	/**< Network EDGE */
	SVCTYPE_3G,		/**< Network UMTS */
	SVCTYPE_HSDPA		/**< Network HSDPA */
} agps_svc_type_t;

/**
 * SUPL network-initiated information
 */
typedef struct {
	char *msg_body;		/**< SUPL NI message body */
	int msg_size;		/**< SUPL NI message size */
	int status;			/**< Return code of Status */
} agps_supl_ni_info_t;

/**
 * Geofence action data type
 */
typedef struct {
	geofence_data_t geofence;
	geofence_zone_state_t last_state;
	int monitor_states;
	int notification_responsiveness_ms;
	int unknown_timer_ms;
} geofence_action_data_t;

typedef struct {
	int interval;
	int period;
} gps_action_start_data_t;

typedef struct {
	int interval;
} gps_action_change_interval_data_t;

/**
 * gps-plugin plugin interface
 */
typedef struct {
	/** Initialize plugin module and register callback function for event delivery. */
	int (*init)(gps_event_cb gps_event_cb, void *user_data);

	/** Deinitialize plugin module */
	int (*deinit)(gps_failure_reason_t *reason_code);

	/** Request specific action to plugin module */
	int (*request)(gps_action_t gps_action, void *gps_action_data, gps_failure_reason_t *reason_code);
} gps_plugin_interface;

const gps_plugin_interface *get_gps_plugin_interface();

#ifdef __cplusplus
}
#endif
#endif /* _GPS_PLUGIN_INTF_H_ */
