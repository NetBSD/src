Name:           tpm-tools
Version:        1.2.4
Release:        1
Summary:        Management tools for the TPM hardware

Group:          Applications/System
License:        CPL
URL:            http://www.sf.net/projects/trousers
Source0:        %{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  autoconf automake libtool trousers-devel openssl-devel perl
Requires:       trousers

%description
tpm-tools is a group of tools to manage and utilize the Trusted Computing
Group's TPM hardware. TPM hardware can create, store and use RSA keys
securely (without ever being exposed in memory), verify a platform's
software state using cryptographic hashes and more.

%package devel
Summary:	Files to use the library routines supplied with tpm-tools
Group:		Development/Libraries
Requires:	tpm-tools = %{version}-%{release} 

%description devel
tpm-tools-devel is a package that contains the libraries and headers
necessary for developing tpm-tools applications.


%prep
%setup -q


%build
autoreconf --force --install
%configure --disable-static --disable-pkcs11-support
make


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
rm -f $RPM_BUILD_ROOT/%{_libdir}/libtpm_unseal.la


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc LICENSE README
%attr(755, root, root) %{_bindir}/tpm_*
%attr(755, root, root) %{_sbindir}/tpm_*
%attr(755, root, root) %{_libdir}/libtpm_unseal.so.0.0.1
%{_libdir}/libtpm_unseal.so.0
%{_mandir}/man1/tpm_*
%{_mandir}/man8/tpm_*

%files devel
%defattr(-,root,root,-)
%{_libdir}/libtpm_unseal.so
%{_includedir}/tpm_tools/*.h
%{_mandir}/man3/tpmUnseal*


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig
