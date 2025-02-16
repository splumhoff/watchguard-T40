#
# spec file for package ipsec-tools
#
# Copyright (c) 2005 SUSE LINUX AG, Nuernberg, Germany.
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments via http://www.suse.de/feedback/
#

# norootforbuild
# neededforbuild  kernel-source openssl openssl-devel readline-devel

BuildRequires: aaa_base acl attr bash bind-utils bison bzip2 coreutils cpio cpp cracklib cvs cyrus-sasl db devs diffutils e2fsprogs file filesystem fillup findutils flex gawk gdbm-devel glibc glibc-devel glibc-locale gpm grep groff gzip info insserv less libacl libattr libgcc libselinux libstdc++ libxcrypt libzio m4 make man mktemp module-init-tools ncurses ncurses-devel net-tools netcfg openldap2-client openssl pam pam-modules patch permissions popt procinfo procps psmisc pwdutils rcs readline sed strace syslogd sysvinit tar tcpd texinfo timezone unzip util-linux vim zlib zlib-devel autoconf automake binutils gcc gdbm gettext kernel-source libtool openssl-devel perl readline-devel rpm

Name:         ipsec-tools
Version:      0.7.2
Release:      0
License:      Other License(s), see package, BSD
Group:        Productivity/Networking/Security
Provides:     racoon
PreReq:       %insserv_prereq %fillup_prereq
Autoreqprov:  on
Summary:      IPsec Utilities
Source:       http://prdownloads.sourceforge.net/ipsec-tools/ipsec-tools-%{version}.tar.bz2
Source1:      racoon.init
Source2:      sysconfig.racoon
URL:          http://ipsec-tools.sourceforge.net/
Prefix:       /usr
BuildRoot:    %{_tmppath}/%{name}-%{version}-build

%description
This is the IPsec-Tools package.  This package is needed to really make
use of the IPsec functionality in the version 2.5 and 2.6 Linux
kernels.  This package builds:

- libipsec, a PFKeyV2 library

- setkey, a program to directly manipulate policies and SAs

- racoon, an IKEv1 keying daemon

These sources can be found at the IPsec-Tools home page at:
http://ipsec-tools.sourceforge.net/



Authors:
--------
    Derek Atkins  <derek@ihtfp.com>
    Michal Ludvig <mludvig@suse.cz>

%prep
%setup

%build
%{suse_update_config -f . src/racoon}
CFLAGS="$RPM_OPT_FLAGS" \
./configure --prefix=/usr --disable-shared \
	--mandir=%{_mandir} --infodir=%{_infodir} --libdir=%{_libdir} \
	--libexecdir=%{_libdir} --sysconfdir=/etc/racoon \
	--sharedstatedir=/var/run --localstatedir=/var \
	--enable-dpd --enable-hybrid --enable-frag
make 
make check

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc/init.d
install -m 0755 $RPM_SOURCE_DIR/racoon.init $RPM_BUILD_ROOT/etc/init.d/racoon
ln -sf /etc/init.d/racoon $RPM_BUILD_ROOT/usr/sbin/rcracoon
mkdir -p $RPM_BUILD_ROOT/var/adm/fillup-templates
install -m 644 $RPM_SOURCE_DIR/sysconfig.racoon $RPM_BUILD_ROOT/var/adm/fillup-templates/
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/packages/%{name}/
cp -rv src/racoon/samples $RPM_BUILD_ROOT/usr/share/doc/packages/%{name}/
cp -v src/setkey/sample* $RPM_BUILD_ROOT/usr/share/doc/packages/%{name}/

%post
%{fillup_and_insserv racoon}

%postun
%{insserv_cleanup}

%clean
if test ! -z "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != "/"; then
  rm -rf $RPM_BUILD_ROOT
fi

%files
%defattr(-,root,root)
%dir /etc/racoon
%config(noreplace) /etc/racoon/psk.txt
%config(noreplace) /etc/racoon/racoon.conf
%config(noreplace) /etc/racoon/setkey.conf
%config /etc/init.d/racoon
/usr/sbin/rcracoon
%dir /usr/include/libipsec/
%doc /usr/share/doc/packages/%{name}/
/var/adm/fillup-templates/sysconfig.racoon
/usr/include/libipsec/libpfkey.h
/usr/%{_lib}/libipsec.a
/usr/%{_lib}/libipsec.la
/usr/sbin/racoon
/usr/sbin/racoonctl
/usr/sbin/setkey
/usr/sbin/plainrsa-gen
%{_mandir}/man*/*

%changelog -n ipsec-tools
