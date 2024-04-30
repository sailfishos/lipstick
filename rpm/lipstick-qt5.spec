Name:       lipstick-qt5

# We need this folder, so that lipstick can monitor it. See the code
# in src/components/launchermodel.cpp for reference.
%define icondirectory %{_datadir}/icons/hicolor/86x86/apps

Summary:    QML toolkit for homescreen creation
Version:    0.36.29
Release:    1
License:    LGPLv2
URL:        https://github.com/sailfishos/lipstick/
Source0:    %{name}-%{version}.tar.bz2
Source1:    %{name}.privileges
Requires:   mce >= 1.87.0
Requires:   pulseaudio-modules-nemo-mainvolume >= 6.0.19
Requires:   user-managerd >= 0.3.0
Requires:   sailjail-daemon
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Quick) >= 5.2.1
BuildRequires:  pkgconfig(Qt5Sql)
BuildRequires:  pkgconfig(Qt5Test)
BuildRequires:  pkgconfig(Qt5Sensors)
BuildRequires:  pkgconfig(contentaction5)
BuildRequires:  pkgconfig(mlite5) >= 0.2.19
BuildRequires:  pkgconfig(mce) >= 1.22.0
BuildRequires:  pkgconfig(mce-qt5) >= 1.5.0
BuildRequires:  pkgconfig(keepalive)
BuildRequires:  pkgconfig(dsme_dbus_if) >= 0.63.2
BuildRequires:  pkgconfig(thermalmanager_dbus_if)
BuildRequires:  pkgconfig(usb_moded)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(libresourceqt5)
BuildRequires:  pkgconfig(ngf-qt5)
BuildRequires:  pkgconfig(systemd)
BuildRequires:  pkgconfig(wayland-server)
BuildRequires:  pkgconfig(usb-moded-qt5) >= 1.8
BuildRequires:  pkgconfig(systemsettings) >= 0.8.0
BuildRequires:  pkgconfig(nemodevicelock)
BuildRequires:  pkgconfig(nemoconnectivity)
BuildRequires:  pkgconfig(sailfishusermanager)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  qt5-qttools-linguist
BuildRequires:  qt5-qtgui-devel >= 5.2.1+git24
BuildRequires:  qt5-qtwayland-wayland_egl-devel >= 5.4.0+git26
BuildRequires:  doxygen
BuildRequires:  qt5-qttools-qthelp-devel
BuildRequires:  nemo-qml-plugin-systemsettings >= 0.8.0

%description
A QML toolkit for homescreen creation

%package devel
Summary:    Development files for lipstick
Requires:   %{name} = %{version}-%{release}

%description devel
Files useful for building homescreens.

%package tests
Summary:    Tests for lipstick
Requires:   %{name} = %{version}-%{release}
Requires:   nemo-test-tools

%description tests
Unit tests for the lipstick package.

%package tools
Summary:    Tools for lipstick
Requires:   %{name} = %{version}-%{release}

%description tools
Tools for the lipstick package (warning: these tools installed by default).

%package simplecompositor
Summary:    Lipstick Simple Compositor
Requires:   %{name} = %{version}-%{release}

%description simplecompositor
Debugging tool to debug the compositor logic without pulling in all of the
homescreen and all the other app logic lipstick has.

%package doc
Summary:    Documentation for lipstick
BuildArch:  noarch

%description doc
Documentation for the lipstick package.

%package notification-doc
Summary:    Documentation for lipstick notification services
BuildArch:  noarch

%description notification-doc
Documentation for the lipstick notification services.

%package ts-devel
Summary:    Translation files for lipstick
BuildArch:  noarch

%description ts-devel
Translation files for the lipstick package.

%prep
%setup -q -n %{name}-%{version}

%build

%qmake5 VERSION=%{version}
%make_build

%install
mkdir -p %{buildroot}/%{icondirectory}
%qmake5_install

mkdir -p %{buildroot}%{_datadir}/mapplauncherd/privileges.d
install -m 644 -p %{SOURCE1} %{buildroot}%{_datadir}/mapplauncherd/privileges.d/

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%license LICENSE.LGPL
%config %{_sysconfdir}/dbus-1/system.d/lipstick.conf
%{_libdir}/lib%{name}.so.*
%{_libdir}/qt5/qml/org/nemomobile/lipstick
%{_datadir}/translations/*.qm
%{_datadir}/lipstick
%{_datadir}/mapplauncherd/privileges.d/*
%dir %{icondirectory}

%files devel
%{_includedir}/%{name}
%{_libdir}/lib%{name}.so
%{_libdir}/lib%{name}.prl
%{_libdir}/pkgconfig/%{name}.pc

%files tests
/opt/tests/lipstick-tests

%files tools
%{_bindir}/notificationtool

%files simplecompositor
%{_bindir}/simplecompositor
%{_datadir}/lipstick/simplecompositor

%files doc
%{_datadir}/doc/lipstick

%files notification-doc
%{_datadir}/doc/lipstick-notification

%files ts-devel
%{_datadir}/translations/source/*.ts
