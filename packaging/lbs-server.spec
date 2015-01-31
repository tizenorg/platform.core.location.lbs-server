Name:       lbs-server
Summary:    lbs server for Tizen
Version:    0.6.0
Release:    1
Group:      TO_BE/FILLED_IN
License:    TO_BE/FILLED_IN
Source0:    %{name}-%{version}.tar.gz
Source1:    lbs-server.service
BuildRequires:  model-build-features
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(network)
BuildRequires:  pkgconfig(tapi)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(location)
BuildRequires:  pkgconfig(lbs-dbus)
BuildRequires:  pkgconfig(gio-unix-2.0)
BuildRequires:  pkgconfig(capi-network-wifi)
#%if "%{_repository}" == "wearable“
#BuildRequires:  pkgconfig(journal)
#BuildRequires:  pkgconfig(libresourced)
#BuildRequires:  pkgconfig(capi-appfw-app-manager)
#BuildRequires:  pkgconfig(deviced)
#%endif
Requires:  sys-assert

%description
lbs server for Tizen


%package -n location-lbs-server
Summary:    lbs server for Tizen
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n location-lbs-server
lbs server for Tizen


%package -n lbs-server-plugin-devel
Summary:    lbs server for Tizen (development files)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n lbs-server-plugin-devel
lbs server for Tizen (development files)


%prep
%setup -q

%ifarch %{arm}
%define GPS_ENABLER --enable-gps
%define ARCH armel
%else
%define GPS_ENABLER --enable-gps
%define ARCH x86
%endif

%if 0%{?model_build_feature_location_position_wps}
%define WPS_ENABLER --enable-wps=yes
%else
%define WPS_ENABLER --enable-wps=no
%endif

%build
#export CFLAGS="$CFLAGS -D_TIZEN_PUBLIC_"

#%if "%{_repository}" == "wearable“
#export CFLAGS="$CFLAGS -DTIZEN_WEARABLE"
#%endif

export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"

./autogen.sh
./configure --prefix=%{_prefix} %{GPS_ENABLER} %{WPS_ENABLER} \

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}/usr/lib/systemd/system/multi-user.target.wants
install -m 644 %{SOURCE1} %{buildroot}/usr/lib/systemd/system/lbs-server.service
ln -s ../lbs-server.service %{buildroot}/usr/lib/systemd/system/multi-user.target.wants/lbs-server.service

%define GPS_DUMP_DIR /opt/etc/dump.d/module.d

mkdir -p %{buildroot}/%{GPS_DUMP_DIR}
cp -a lbs-server/script/dump_gps.sh %{buildroot}/%{GPS_DUMP_DIR}/dump_gps.sh

%clean
rm -rf %{buildroot}


%post
#GPS Indicator value
vconftool set -t int memory/location/position/state "0" -i -s location_fw::vconf
vconftool set -t int memory/location/gps/state "0" -i -s location_fw::vconf
vconftool set -t int memory/location/wps/state "0" -i -s location_fw::vconf

#NMEA_SETTING
vconftool set -t int db/location/nmea/LoggingEnabled "0" -f -s location_fw::vconf -u 5000

#REPLAY_SETTING
vconftool set -t string db/location/replay/FileName "nmea_replay.log" -f -s location_fw::vconf -u 5000
%ifarch %arm
	vconftool set -t int db/location/replay/ReplayEnabled "0" -f -s location_fw::vconf -u 5000
	vconftool set -t int db/location/replay/ReplayMode "1" -f -s location_fw::vconf -u 5000
%else
	vconftool set -t int db/location/replay/ReplayEnabled "1" -f -s location_fw::vconf -u 5000
	vconftool set -t int db/location/replay/ReplayMode "0" -f -s location_fw::vconf -u 5000
%endif
vconftool set -t double db/location/replay/ManualLatitude "0.0" -f -s location_fw::vconf -u 5000
vconftool set -t double db/location/replay/ManualLongitude "0.0" -f -s location_fw::vconf -u 5000
vconftool set -t double db/location/replay/ManualAltitude "0.0" -f -s location_fw::vconf -u 5000
vconftool set -t double db/location/replay/ManualHAccuracy "0.0" -f -s location_fw::vconf -u 5000

%post -n location-lbs-server
#%ifnarch %arm
#	cp -f /usr/lib/location/module/libgps.so /usr/lib/location/module/libwps0.so
#%endif

%postun -p /sbin/ldconfig

%files
%manifest lbs-server.manifest
%defattr(-,root,root,-)
/usr/libexec/lbs-server
/usr/share/dbus-1/services/org.tizen.lbs.Providers.LbsServer.service
/usr/share/lbs/lbs-server.provider
/etc/rc.d/init.d/lbs-server
/etc/rc.d/rc5.d/S90lbs-server
/usr/lib/systemd/system/lbs-server.service
/usr/lib/systemd/system/multi-user.target.wants/lbs-server.service
/opt/etc/dump.d/module.d/dump_gps.sh
#%if "%{_repository}" == "wearable“
#/usr/share/packages/com.samsung.lbs-server.xml
#%endif

%files -n location-lbs-server
%manifest location-lbs-server.manifest
%defattr(-,root,root,-)
%{_libdir}/location/module/libgps.so*

%if 0%{?model_build_feature_location_position_wps}
%{_libdir}/location/module/libwps.so*
%endif

%files -n lbs-server-plugin-devel
%defattr(-,root,root,-)
#%if "%{_repository}" == "wearable“
%{_libdir}/location/module/*.so
#%endif
%{_libdir}/pkgconfig/lbs-server-plugin.pc
%{_includedir}/lbs-server-plugin/*.h
