#!/bin/bash

/usr/bin/vconftool set -t int "db/location/setting/Usemylocation"  "1" -s "tizen::vconf::location::enable" -g 6514
/usr/bin/vconftool set -t int "db/location/setting/GpsEnabled"  "1" -s "tizen::vconf::location::enable" -g 6514
/usr/bin/vconftool set -t int "db/location/setting/NetworkEnabled"  "0" -s "tizen::vconf::location::enable" -g 6514
/usr/bin/vconftool set -t int "db/location/setting/GpsPopup"  "1" -s "tizen::vconf::location::enable" -g 6514
/usr/bin/vconftool set -t int "memory/location/position/state"  "0" -s "tizen::vconf::public::r::platform::rw" -i  -g 6514
/usr/bin/vconftool set -t int "memory/location/gps/state"  "0" -s "tizen::vconf::public::r::platform::rw" -i  -g 6514
/usr/bin/vconftool set -t int "memory/location/wps/state"  "0" -s "tizen::vconf::public::r::platform::rw" -i  -g 6514
/usr/bin/vconftool set -t int "memory/location/last/gps/Timestamp"  "0" -s "tizen::vconf::location" -i  -g 6514
/usr/bin/vconftool set -t double "memory/location/last/gps/Latitude"  "0.0" -s "tizen::vconf::location" -i  -g 6514
/usr/bin/vconftool set -t double "memory/location/last/gps/Longitude"  "0.0" -s "tizen::vconf::location" -i  -g 6514
/usr/bin/vconftool set -t double "memory/location/last/gps/Altitude"  "0.0" -s "tizen::vconf::location" -i  -g 6514
/usr/bin/vconftool set -t double "memory/location/last/gps/Speed"  "0.0" -s "tizen::vconf::location" -i  -g 6514
/usr/bin/vconftool set -t double "memory/location/last/gps/Direction"  "0.0" -s "tizen::vconf::location" -i  -g 6514
/usr/bin/vconftool set -t double "memory/location/last/gps/HorAccuracy"  "0.0" -s "tizen::vconf::location" -i  -g 6514
/usr/bin/vconftool set -t double "memory/location/last/gps/VerAccuracy"  "0.0" -s "tizen::vconf::location" -i  -g 6514
/usr/bin/vconftool set -t double "memory/location/last/wps/Timestamp"  "0.0" -s "tizen::vconf::location" -i  -g 6514
/usr/bin/vconftool set -t double "memory/location/last/wps/Latitude"  "0.0" -s "tizen::vconf::location" -i  -g 6514
/usr/bin/vconftool set -t double "memory/location/last/wps/Longitude"  "0.0" -s "tizen::vconf::location" -i  -g 6514
/usr/bin/vconftool set -t double "memory/location/last/wps/Altitude"  "0.0" -s "tizen::vconf::location" -i  -g 6514
/usr/bin/vconftool set -t double "memory/location/last/wps/Speed"  "0.0" -s "tizen::vconf::location" -i  -g 6514
/usr/bin/vconftool set -t double "memory/location/last/wps/Direction"  "0.0" -s "tizen::vconf::location" -i  -g 6514
/usr/bin/vconftool set -t double "memory/location/last/wps/HorAccuracy"  "0.0" -s "tizen::vconf::location" -i  -g 6514
/usr/bin/vconftool set -t int "db/location/last/gps/LocTimestamp"  "0" -s "tizen::vconf::location" -g 6514
/usr/bin/vconftool set -t int "db/location/last/wps/LocTimestamp"  "0" -s "tizen::vconf::location" -g 6514
/usr/bin/vconftool set -t string "db/location/last/gps/Location"  "0.0;0.0;0.0;0.0;0.0;0.0;0.0;" -s "tizen::vconf::location"  -g 6514
/usr/bin/vconftool set -t string "db/location/last/wps/Location"  "0.0;0.0;0.0;0.0;0.0;0.0;" -s "tizen::vconf::location"  -g 6514
/usr/bin/vconftool set -t int "db/location/nmea/LoggingEnabled"  "0" -s "tizen::vconf::platform::rw" -g 6514
/usr/bin/vconftool set -t int "db/location/replay/ReplayEnabled"  "0" -s "tizen::vconf::platform::rw" -g 6514
/usr/bin/vconftool set -t int "db/location/replay/ReplayMode"  "1" -s "tizen::vconf::platform::rw" -g 6514
/usr/bin/vconftool set -t string "db/location/replay/FileName"  "nmea_replay.log" -s "tizen::vconf::platform::rw" -g 6514
/usr/bin/vconftool set -t double "db/location/replay/ManualLatitude"  "0.0" -s "tizen::vconf::platform::rw" -g 6514
/usr/bin/vconftool set -t double "db/location/replay/ManualLongitude"  "0.0" -s "tizen::vconf::platform::rw" -g 6514
/usr/bin/vconftool set -t double "db/location/replay/ManualAltitude"  "0.0" -s "tizen::vconf::platform::rw"  -g 6514
/usr/bin/vconftool set -t double "db/location/replay/ManualHAccuracy"  "0.0" -s "tizen::vconf::platform::rw"  -g 6514
/usr/bin/vconftool set -t int "db/location/gps/Operation"  "1" -s "tizen::vconf::platform::rw"  -g 6514
/usr/bin/vconftool set -t int "db/location/gps/OperationTest"  "0" -s "tizen::vconf::platform::rw"  -g 6514
/usr/bin/vconftool set -t int "db/location/gps/Starting"  "0" -s "tizen::vconf::platform::rw"  -g 6514
/usr/bin/vconftool set -t int "db/location/gps/Session"  "1" -s "tizen::vconf::platform::rw"  -g 6514
/usr/bin/vconftool set -t double "db/location/gps/XtraDownloadTime"  "0.0" -s "tizen::vconf::platform::rw"  -g 6514
/usr/bin/vconftool set -t string "db/location/supl/Server"  "" -s "tizen::vconf::platform::rw"  -g 6514
/usr/bin/vconftool set -t string "db/location/supl/Port"  "7275" -s "tizen::vconf::platform::rw"  -g 6514
/usr/bin/vconftool set -t int "db/location/supl/SslEnabled"  "1" -s "tizen::vconf::platform::rw"   -g 6514
/usr/bin/vconftool set -t int "db/location/supl/FQDNType"  "0" -s "tizen::vconf::platform::rw"  -g 6514
/usr/bin/vconftool set -t int "db/location/supl/Version"  "0" -s "tizen::vconf::platform::rw"  -g 6514
