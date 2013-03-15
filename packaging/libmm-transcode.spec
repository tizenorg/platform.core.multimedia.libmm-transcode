#sbs-git:slp/pkgs/l/libmm-transcode libmm-transcode 0.1 62b62e6d483557fc5750d1b4986e9a98323f1194
Name:       libmm-transcode
Summary:    Multimedia Framework Video Transcode Library
Version:    0.8
Release:    2
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Requires(post):  /sbin/ldconfig
Requires(postun):  /sbin/ldconfig
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(mm-log)
BuildRequires:  pkgconfig(mm-ta)
BuildRequires:  pkgconfig(mm-fileinfo)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gstreamer-0.10)
BuildRequires:  pkgconfig(gstreamer-app-0.10)
BuildRequires:  pkgconfig(gstreamer-interfaces-0.10)
BuildRequires:  pkgconfig(gstreamer-plugins-base-0.10)
BuildRequires:  pkgconfig(gstreamer-pbutils-0.10)
BuildRequires:  pkgconfig(gmodule-2.0)

BuildRoot:  %{_tmppath}/%{name}-%{version}-build

%description

%package devel
Summary:    Multimedia Framework Video Transcode Library (DEV)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel

%package tool
Summary:    Multimedia Framework Video Transcode Utility Package
Group:      TO_BE/FILLED_IN
Requires:   %{name} = %{version}-%{release}

%description tool

%prep
%setup -q

%build
./autogen.sh

CFLAGS="$CFLAGS -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" -D_MM_PROJECT_FLOATER" \
LDFLAGS+="-Wl,--rpath=%{_prefix}/lib -Wl,--hash-style=both -Wl,--as-needed" \
./configure --prefix=%{_prefix}

make %{?jobs:-j%jobs}

sed -i -e "s#@TRANSCODE_REQPKG@#$TRANSCODE_REQPKG#g" transcode/mm-transcode.pc

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/share/license
cp LICENSE.APLv2.0 %{buildroot}/usr/share/license/%{name}

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig


%files
/usr/share/license/%{name}
%manifest libmm-transcode.manifest
%defattr(-,root,root,-)
%{_libdir}/*.so*

%files devel
%defattr(-,root,root,-)
%{_includedir}/mmf/*.h
%{_libdir}/pkgconfig/*

%files tool
%defattr(-,root,root,-)
%{_bindir}/mm_transcode_testsuite

