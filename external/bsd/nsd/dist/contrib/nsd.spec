Summary: NSD is a complete implementation of an authoritative DNS name server
Name: nsd
Version: 4.1.6
Release: 1%{?dist}
License: BSD
Url: http://www.nlnetlabs.nl/%{name}/
#Source: http://www.nlnetlabs.nl/downloads/%{name}/%{name}-%{version}.tar.gz
Source: %{name}-%{version}.tar.gz
Source1: nsd.init
Source2: nsd.cron
Group: System Environment/Daemons
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: flex, openssl-devel
Requires(pre): shadow-utils

%description
NSD is a complete implementation of an authoritative DNS name server.
It can function as a primary or secondary DNS server, with DNSSEC support.
For further information about what NSD is and what NSD is not please
consult the REQUIREMENTS document which is a part of this distribution.
(thanks to Olaf).

%prep
%setup -q -n %{name}-%{version}

%build
%configure --enable-pie --enable-relro-now --enable-ratelimit \
           --enable-bind8-stats --enable-plugins --enable-checking \
           --enable-mmap --with-ssl --enable-nsec3 --enable-nsid \
           --with-pidfile=%{_localstatedir}/run/%{name}/%{name}.pid --with-ssl \
           --with-user=nsd --with-xfrdfile=%{_localstatedir}/lib/%{name}/ixfr.state

%{__make} %{?_smp_mflags}
#convert to utf8
iconv -f iso8859-1 -t utf-8 doc/RELNOTES > doc/RELNOTES.utf8
iconv -f iso8859-1 -t utf-8 doc/CREDITS > doc/CREDITS.utf8
mv -f doc/RELNOTES.utf8 doc/RELNOTES
mv -f doc/CREDITS.utf8 doc/CREDITS


%install
rm -rf %{buildroot}
%{__make} DESTDIR=%{buildroot} install
install -d -m 0755 %{buildroot}%{_initrddir}
install -d -m 0755 $RPM_BUILD_ROOT%{_sysconfdir}/cron.hourly
install -c -m 0755 %{SOURCE2} $RPM_BUILD_ROOT%{_sysconfdir}/cron.hourly/nsd
install -m 0755 %{SOURCE1} %{buildroot}/%{_initrddir}/nsd
install -d -m 0700 %{buildroot}%{_localstatedir}/run/%{name}
install -d -m 0700 %{buildroot}%{_localstatedir}/lib/%{name}

# change .sample to normal config files
head -76 %{buildroot}%{_sysconfdir}/nsd/nsd.conf.sample > %{buildroot}%{_sysconfdir}/nsd/nsd.conf
rm %{buildroot}%{_sysconfdir}/nsd/nsd.conf.sample
echo "database: /var/lib/nsd/nsd.db" >> %{buildroot}%{_sysconfdir}/nsd/nsd.conf
echo "# include: \"/some/path/file\"" >> %{buildroot}%{_sysconfdir}/nsd/nsd.conf

%clean
rm -rf ${RPM_BUILD_ROOT}

%files
%defattr(-,root,root,-)
%doc doc/*
%doc contrib/nsd.zones2nsd.conf
%dir %{_sysconfdir}/nsd/
%attr(0644,root,root) %config(noreplace) %{_sysconfdir}/nsd/nsd.conf
#%attr(0644,root,root) %config(noreplace) %{_sysconfdir}/nsd/nsd.zones
%attr(0755,root,root) %{_initrddir}/%{name}
%{_sysconfdir}/cron.hourly/nsd
%attr(0755,%{name},%{name}) %dir %{_localstatedir}/run/%{name}
%attr(0755,%{name},%{name}) %dir %{_localstatedir}/lib/%{name}
%{_sbindir}/*
%{_mandir}/*/*

%pre
getent group nsd >/dev/null || groupadd -r nsd
getent passwd nsd >/dev/null || \
useradd -r -g nsd -d /etc/nsd -s /sbin/nologin \
-c "nsd daemon account" nsd
exit 0

%post
/sbin/chkconfig --add %{name}

%preun
if [ $1 -eq 0 ]; then
        /sbin/service %{name} stop
        /sbin/chkconfig --del %{name}
fi

%postun
if [ "$1" -ge "1" ]; then
  /sbin/service %{name} condrestart
fi
