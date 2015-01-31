#!/bin/sh

#--------------------------------------
#	GPS
#--------------------------------------

GPS_DEBUG_DIR=$1/lbs_server
mkdir -p ${GPS_DEBUG_DIR}

cp /tmp/dump_lbs.log ${GPS_DEBUG_DIR}/

vconftool get memory/location/gps >> ${GPS_DEBUG_DIR}/vconf
vconftool get memory/location/wps >> ${GPS_DEBUG_DIR}/vconf
vconftool get db/location/setting >> ${GPS_DEBUG_DIR}/vconf
