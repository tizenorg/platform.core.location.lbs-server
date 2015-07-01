Name:       lbs-server
Summary:    LBS Server for Tizen
Version:    0.6.8
Release:    1
Group:		Location/Service
License:	Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    lbs-server.service
Source2:    lbs-server.manifest
Source3:    location-lbs-server.manifest
BuildRequires:  cmake
#BuildRequires:  model-build-features
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(network)
BuildRequires:  pkgconfig(tapi)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(lbs-location)
BuildRequires:  pkgconfig(lbs-dbus)
BuildRequires:  pkgconfig(gio-unix-2.0)
BuildRequires:  pkgconfig(capi-network-wifi)
BuildRequires:  pkgconfig(gio-2.0)
BuildRequires:  pkgconfig(vconf-internal-keys)
BuildRequires:  pkgconfig(gthread-2.0)
BuildRequires:  pkgconfig(gmodule-2.0)
Requires:  sys-assert

%description
LBS Server for Tizen
LBS Server provides geographical location information


%package -n location-lbs-server
Summary:    Client of LBS Server for Tizen
Group:      Location/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n location-lbs-server
Client of LBS Server for Tizen
This package provides geographical location information received from LBS Server


%package -n lbs-server-plugin-devel
Summary:    LBS Server plugin for Tizen (Development)
Group:      Location/Development
Requires:   %{name} = %{version}-%{release}

%description -n lbs-server-plugin-devel
LBS Server plugin for Tizen (Development)
This package provides header files and pkgconfig file for LBS Server plugin

%prep
%setup -q
cp %{SOURCE1} .
cp %{SOURCE2} .
cp %{SOURCE3} .

%ifarch %{arm}
%define ARCH armel
%else
%define ARCH x86
%endif

%build
%define _prefix /usr

export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"

MAJORVER=`echo %{version} | awk 'BEGIN {FS="."}{print $1}'`
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DFULLVER=%{version} -DMAJORVER=${MAJORVER} \
        -DLIB_DIR=%{_libdir} -DINCLUDE_DIR=%{_includedir} -DSYSCONF_DIR=%{_sysconfdir} \
#%if 0%{?model_build_feature_location_position_wps}
#    -DENABLE_WPS=YES \
#%endif

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants
install -m 644 %{SOURCE1} %{buildroot}%{_libdir}/systemd/system/lbs-server.service
ln -s ../lbs-server.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/lbs-server.service

chmod 755 %{buildroot}/etc/rc.d/init.d/lbs-server
mkdir -p %{buildroot}/etc/rc.d/rc5.d
ln -sf ../init.d/lbs-server %{buildroot}/etc/rc.d/rc5.d/S90lbs-server

%define GPS_DUMP_DIR /opt/etc/dump.d/module.d

mkdir -p %{buildroot}/%{GPS_DUMP_DIR}
cp -a lbs-server/script/dump_gps.sh %{buildroot}/%{GPS_DUMP_DIR}/dump_gps.sh

%clean
rm -rf %{buildroot}


%post

%ifarch %arm
	vconftool set -t int "db/location/replay/ReplayEnabled" "0" -s "tizen::vconf::platform::rw" -g 6514 -f
	vconftool set -t int "db/location/replay/ReplayMode" "1" -s "tizen::vconf::platform::rw" -g 6514 -f
%else
	vconftool set -t int "db/location/replay/ReplayEnabled" "1" -s "tizen::vconf::platform::rw" -g 6514 -f
	vconftool set -t int "db/location/replay/ReplayMode" "0" -s "tizen::vconf::platform::rw" -g 6514 -f
%endif

%post -n location-lbs-server
#%ifnarch %arm
#	cp -f /usr/lib/location/module/libgps.so /usr/lib/location/module/libwps0.so
#%endif

%postun -p /sbin/ldconfig

%files
%manifest lbs-server.manifest
%license LICENSE
%defattr(-,system,system,-)
%{_bindir}/lbs-server
/usr/share/dbus-1/system-services/org.tizen.lbs.Providers.LbsServer.service
#/usr/share/lbs/lbs-server.provider
%config %{_sysconfdir}/dbus-1/system.d/lbs-server.conf
/etc/rc.d/init.d/lbs-server
/etc/rc.d/rc5.d/S90lbs-server
%{_libdir}/systemd/system/lbs-server.service
%{_libdir}/systemd/system/multi-user.target.wants/lbs-server.service
/opt/etc/dump.d/module.d/dump_gps.sh

%files -n location-lbs-server
%manifest location-lbs-server.manifest
%{_libdir}/location/module/libgps.so*

%if 0%{?model_build_feature_location_position_wps}
%{_libdir}/location/module/libwps.so*
%endif

%files -n lbs-server-plugin-devel
%{_libdir}/pkgconfig/lbs-server-plugin.pc
%{_includedir}/lbs-server-plugin/*.h
