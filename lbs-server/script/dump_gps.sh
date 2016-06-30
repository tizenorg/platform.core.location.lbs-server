#!/bin/sh
PATH=/bin:/usr/bin:/sbin:/usr/sbin

#--------------------------------------
#	GPS
#--------------------------------------

GPS_DEBUG_DIR=$1/gps
mkdir -p ${GPS_DEBUG_DIR}

cp /tmp/dump_gps.log ${GPS_DEBUG_DIR}/

vconftool get memory/location/gps >> ${GPS_DEBUG_DIR}/vconf
vconftool get memory/location/wps >> ${GPS_DEBUG_DIR}/vconf
vconftool get db/location/setting >> ${GPS_DEBUG_DIR}/vconf
