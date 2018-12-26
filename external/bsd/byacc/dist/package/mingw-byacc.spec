Summary: byacc - public domain Berkeley LALR Yacc parser generator
%define AppProgram byacc
%define AppVersion 20180609
%define UseProgram yacc
# Id: mingw-byacc.spec,v 1.23 2018/06/10 00:42:16 tom Exp 
Name: %{AppProgram}
Version: %{AppVersion}
Release: 1
License: Public Domain, MIT
Group: Applications/Development
URL: ftp://invisible-island.net/%{AppProgram}
Source0: %{AppProgram}-%{AppVersion}.tgz
Packager: Thomas Dickey <dickey@invisible-island.net>

%description
This package provides a parser generator utility that reads a grammar
specification from a file and generates an LR(1) parser for it.  The
parsers consist of a set of LALR(1) parsing tables and a driver
routine written in the C programming language.  It has a public domain
license which includes the generated C.

%prep

%define debug_package %{nil}

%setup -q -n %{AppProgram}-%{AppVersion}

%build
%configure --verbose \
		--program-prefix=b \
		--target %{_target_platform} \
		--prefix=%{_prefix} \
		--bindir=%{_bindir} \
		--libdir=%{_libdir} \
		--mandir=%{_mandir}

make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

make install                    DESTDIR=$RPM_BUILD_ROOT
( cd $RPM_BUILD_ROOT%{_bindir} && ln -s %{AppProgram} %{UseProgram} )

strip $RPM_BUILD_ROOT%{_bindir}/%{AppProgram}

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_prefix}/bin/%{AppProgram}
%{_prefix}/bin/%{UseProgram}
%{_mandir}/man1/%{AppProgram}.*

%changelog
# each patch should add its ChangeLog entries here

* Sun Jul 09 2017 Thomas Dickey
- use predefined "configure"

* Wed Sep 25 2013 Thomas Dickey
- cloned from byacc.spec
