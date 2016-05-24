Name:    fused-location
Summary: Fused Location service for Tizen
Version: 0.0.3
Release: 1
Group:   Location/Service
License: Apache-2.0
Source0: %{name}-%{version}.tar.gz
Source1: fused-location.service
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires: cmake
#Requires:      gtest
#BuildRequires: gtest-devel
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(gobject-2.0)
BuildRequires: pkgconfig(gio-2.0)
BuildRequires: pkgconfig(gio-unix-2.0)
BuildRequires: pkgconfig(capi-appfw-app-manager)
BuildRequires: pkgconfig(capi-system-sensor)
BuildRequires: pkgconfig(lbs-dbus)

%description
Fused Location service for Tizen
Fused Location provides geographical location.

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%{_bindir}/fused-location*
%{_libdir}/libfused-location*
%{_libdir}/pkgconfig/fused-location-client.pc
%{_includedir}/fused-location/*.h

%prep
%setup -q

%build

%{!?build_type:%define build_type "RELEASE"}

%if %{build_type} == "DEBUG"
    CFLAGS="$CFLAGS -Wp,-U_FORTIFY_SOURCE"
    CXXFLAGS="$CXXFLAGS -Wp,-U_FORTIFY_SOURCE"
%endif

%cmake . -DVERSION=%{version} \
         -DCMAKE_BUILD_TYPE=%{build_type} \
         -DBUILD_TESTS=OFF
make -k %{?jobs:-j%jobs}

%install
%make_install

%clean
rm -rf %{buildroot}
