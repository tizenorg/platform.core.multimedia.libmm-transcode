Name:       libmm-transcode
Summary:    Multimedia Framework Video Transcode Library
Version:    0.9
Release:    6
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: 	libmm-transcode.manifest
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(mm-log)
BuildRequires:  pkgconfig(mm-fileinfo)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gstreamer-1.0)
BuildRequires:  pkgconfig(gstreamer-app-1.0)
BuildRequires:  pkgconfig(gstreamer-video-1.0)
BuildRequires:  pkgconfig(gstreamer-plugins-base-1.0)
BuildRequires:  pkgconfig(gstreamer-pbutils-1.0)
BuildRequires:  pkgconfig(gmodule-2.0)

%description
Multimedia Framework Video Transcode Library.

%package devel
Summary:    Multimedia Framework Video Transcode Library (DEV)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Multimedia Framework Video Transcode Library (DEV).

%package tool
Summary:    Multimedia Framework Video Transcode Utility Package
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description tool
Multimedia Framework Video Transcode Utility Package

%prep
%setup -q
cp %{SOURCE1001} .

%build
./autogen.sh

CFLAGS="$CFLAGS -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" -D_MM_PROJECT_FLOATER" \
LDFLAGS+="-Wl,--rpath=%{_libdir} -Wl,--hash-style=both -Wl,--as-needed" \
%configure

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
%manifest %{name}.manifest
/usr/share/license/%{name}
%manifest libmm-transcode.manifest
%defattr(-,root,root,-)
%{_libdir}/*.so*

%files devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/mmf/*.h
%{_libdir}/pkgconfig/*

%files tool
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_bindir}/mm_transcode_testsuite

