<!--
 - Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 -
 - This Source Code Form is subject to the terms of the Mozilla Public
 - License, v. 2.0. If a copy of the MPL was not distributed with this
 - file, You can obtain one at http://mozilla.org/MPL/2.0/.
 -
 - See the COPYRIGHT file distributed with this work for additional
 - information regarding copyright ownership.
-->
# BIND 9

### Contents

1. [Introduction](#intro)
1. [Reporting bugs and getting help](#help)
1. [Contributing to BIND](#contrib)
1. [BIND 9.13 features](#features)
1. [Building BIND](#build)
1. [macOS](#macos)
1. [Dependencies](#dependencies)
1. [Compile-time options](#opts)
1. [Automated testing](#testing)
1. [Documentation](#doc)
1. [Change log](#changes)
1. [Acknowledgments](#ack)

### <a name="intro"/> Introduction

BIND (Berkeley Internet Name Domain) is a complete, highly portable
implementation of the DNS (Domain Name System) protocol.

The BIND name server, `named`, is able to serve as an authoritative name
server, recursive resolver, DNS forwarder, or all three simultaneously.  It
implements views for split-horizon DNS, automatic DNSSEC zone signing and
key management, catalog zones to facilitate provisioning of zone data
throughout a name server constellation, response policy zones (RPZ) to
protect clients from malicious data, response rate limiting (RRL) and
recursive query limits to reduce distributed denial of service attacks,
and many other advanced DNS features.  BIND also includes a suite of
administrative tools, including the `dig` and `delv` DNS lookup tools,
`nsupdate` for dynamic DNS zone updates, `rndc` for remote name server
administration, and more.

BIND 9 began as a complete re-write of the BIND architecture that was
used in versions 4 and 8.  Internet Systems Consortium
([https://www.isc.org](https://www.isc.org)), a 501(c)(3) public benefit
corporation dedicated to providing software and services in support of the
Internet infrastructure, developed BIND 9 and is responsible for its
ongoing maintenance and improvement.  BIND is open source software
licenced under the terms of the Mozilla Public License, version 2.0.

For a summary of features introduced in past major releases of BIND,
see the file [HISTORY](HISTORY.md).

For a detailed list of changes made throughout the history of BIND 9, see
the file [CHANGES](CHANGES). See [below](#changes) for details on the
CHANGES file format.

For up-to-date release notes and errata, see
[http://www.isc.org/software/bind9/releasenotes](http://www.isc.org/software/bind9/releasenotes)

For information about supported platforms, see [PLATFORMS](PLATFORMS.md).

### <a name="help"/> Reporting bugs and getting help

To report non-security-sensitive bugs or request new features, you may
open an Issue in the BIND 9 project on the
[ISC GitLab server](https://gitlab.isc.org) at
[https://gitlab.isc.org/isc-projects/bind9](https://gitlab.isc.org/isc-projects/bind9).

Please note that, unless you explicitly mark the newly created Issue as
"confidential", it will be publicly readable.  Please do not include any
information in bug reports that you consider to be confidential unless
the issue has been marked as such.  In particular, if submitting the
contents of your configuration file in a non-confidential Issue, it is
advisable to obscure key secrets: this can be done automatically by
using `named-checkconf -px`.

If the bug you are reporting is a potential security issue, such as an
assertion failure or other crash in `named`, please do *NOT* use GitLab to
report it. Instead, please send mail to
[security-officer@isc.org](mailto:security-officer@isc.org).

Professional support and training for BIND are available from
ISC at [https://www.isc.org/support](https://www.isc.org/support).

To join the __BIND Users__ mailing list, or view the archives, visit
[https://lists.isc.org/mailman/listinfo/bind-users](https://lists.isc.org/mailman/listinfo/bind-users).

If you're planning on making changes to the BIND 9 source code, you
may also want to join the __BIND Workers__ mailing list, at
[https://lists.isc.org/mailman/listinfo/bind-workers](https://lists.isc.org/mailman/listinfo/bind-workers).

### <a name="contrib"/> Contributing to BIND

ISC maintains a public git repository for BIND; details can be found
at [http://www.isc.org/git/](http://www.isc.org/git/).

Information for BIND contributors can be found in the following files:
- General information: [CONTRIBUTING.md](CONTRIBUTING)
- BIND 9 code style: [doc/dev/style.md](doc/dev/style.md)
- BIND architecture and developer guide: [doc/dev/dev.md](doc/dev/dev.md)

Patches for BIND may be submitted as
[Merge Requests](https://gitlab.isc.org/isc-projects/bind9/merge_requests)
in the [ISC GitLab server](https://gitlab.isc.org) at
at [https://gitlab.isc.org/isc-projects/bind9/merge_requests](https://gitlab.isc.org/isc-projects/bind9/merge_requests).

By default, external contributors don't have ability to fork BIND in the
GitLab server, but if you wish to contribute code to BIND, you may request
permission to do so. Thereafter, you can create git branches and directly
submit requests that they be reviewed and merged.

If you prefer, you may also submit code by opening a
[GitLab Issue](https://gitlab.isc.org/isc-projects/bind9/issues) and
including your patch as an attachment, preferably generated by
`git format-patch`.

### <a name="features"/> BIND 9.13 features

BIND 9.13 is the newest development branch of BIND 9. It includes a
number of changes from BIND 9.12 and earlier releases.  New features
include:

* A new "plugin" mechanism has been added to allow query functionality
  to be extended using dynamically loadable libraries. The "filter-aaaa"
  feature has been removed from named and is now implemented as a plugin.
* Socket and task code has been refactored to improve performance.
* QNAME minimization, as described in RFC 7816, is now supported.
* "Root key sentinel" support, enabling validating resolvers to indicate
  via a special query which trust anchors are configured for the root zone.
* Secondary zones can now be configured as "mirror" zones; their contents
  are transferred in as with traditional slave zones, but are subject to
  DNSSEC validation and are not treated as authoritative data when
  answering. This makes it easier to configure a local copy of the root
  zone as described in RFC 7706.
* The "validate-except" option allows configuration of domains below which
  DNSSEC validation should not be performed.
* The default value of "dnssec-validation" is now "auto".
* IDNA2008 is now supported when linking with `libidn2`.
* "named -V" now outputs the default paths for files used by named
  and other tools.

In addition, workarounds that were formerly in place to enable resolution
of domains whose authoritative servers did not respond to EDNS queries
have been removed. See [https://dnsflagday.net](https://dnsflagday.net)
for more details.

Cryptographic support has been modernized. BIND now uses the
best available pseudo-random number generator for the platform on which
it's built. Very old versions of OpenSSL are no longer supported.
Cryptography is now mandatory: building BIND without DNSSEC is no
longer supported.

Special code to support certain legacy operating systems has also
been removed; see the file [PLATFORMS.md](PLATFORMS.md) for details
of supported platforms. In addition to OpenSSL, BIND now requires
support for IPv6, threads, and standard atomic operations provided
by the C compiler.

### <a name="build"/> Building BIND

Minimally, BIND requires a UNIX or Linux system with an ANSI C compiler,
basic POSIX support, and a 64-bit integer type. Successful builds have been
observed on many versions of Linux and UNIX, including RedHat, Fedora,
Debian, Ubuntu, SuSE, Slackware, FreeBSD, NetBSD, OpenBSD, Mac OS X,
Solaris, HP-UX, and OpenWRT.

BIND requires a cryptography provider library such as OpenSSL or a
hardware service module supporting PKCS#11. On Linux, BIND requires
the `libcap` library to set process privileges, though this requirement
can be overridden by disabling capability support at compile time.
See [Compile-time options](#opts) below for details on other libraries
that may be required to support optional features.

BIND is also available for Windows 2008 and higher.  See
`win32utils/readme1st.txt` for details on building for Windows
systems.

To build on a UNIX or Linux system, use:

		$ ./configure
		$ make

If you're planning on making changes to the BIND 9 source, you should run
`make depend`.  If you're using Emacs, you might find `make tags` helpful.

Several environment variables that can be set before running `configure` will
affect compilation:

|Variable|Description |
|--------------------|-----------------------------------------------|
|`CC`|The C compiler to use.  `configure` tries to figure out the right one for supported systems.|
|`CFLAGS`|C compiler flags.  Defaults to include -g and/or -O2 as supported by the compiler.  Please include '-g' if you need to set `CFLAGS`. |
|`STD_CINCLUDES`|System header file directories.  Can be used to specify where add-on thread or IPv6 support is, for example.  Defaults to empty string.|
|`STD_CDEFINES`|Any additional preprocessor symbols you want defined.  Defaults to empty string. For a list of possible settings, see the file [OPTIONS](OPTIONS.md).|
|`LDFLAGS`|Linker flags. Defaults to empty string.|
|`BUILD_CC`|Needed when cross-compiling: the native C compiler to use when building for the target system.|
|`BUILD_CFLAGS`|Optional, used for cross-compiling|
|`BUILD_CPPFLAGS`||
|`BUILD_LDFLAGS`||
|`BUILD_LIBS`||

#### <a name="macos"> macOS

Building on macOS assumes that the "Command Tools for Xcode" is installed.
This can be downloaded from https://developer.apple.com/download/more/
or if you have Xcode already installed you can run "xcode-select --install".
This will add /usr/include to the system and install the compiler and other
tools so that they can be easily found.

### <a name="dependencies"/> Dependencies

Portions of BIND that are written in Python, including
`dnssec-keymgr`, `dnssec-coverage`, `dnssec-checkds`, and some of the
system tests, require the 'argparse' and 'ply' modules to be available.
'argparse' is a standard module as of Python 2.7 and Python 3.2.
'ply' is available from [https://pypi.python.org/pypi/ply](https://pypi.python.org/pypi/ply).

#### <a name="opts"/> Compile-time options

To see a full list of configuration options, run `configure --help`.

To build shared libraries, specify `--with-libtool` on the `configure`
command line.

For the server to support DNSSEC, you need to build it with crypto support.
To use OpenSSL, you should have OpenSSL 1.0.2e or newer installed.  If the
OpenSSL library is installed in a nonstandard location, specify the prefix
using `--with-openssl=<PREFIX>` on the configure command line. To use a
PKCS#11 hardware service module for cryptographic operations, specify the
path to the PKCS#11 provider library using `--with-pkcs11=<PREFIX>`, and
configure BIND with `--enable-native-pkcs11`.

To support the HTTP statistics channel, the server must be linked with at
least one of the following: libxml2
[http://xmlsoft.org](http://xmlsoft.org) or json-c
[https://github.com/json-c](https://github.com/json-c).  If these are
installed at a nonstandard location, specify the prefix using
`--with-libxml2=/prefix` or `--with-libjson=/prefix`.

To support compression on the HTTP statistics channel, the server must be
linked against libzlib.  If this is installed in a nonstandard location,
specify the prefix using `--with-zlib=/prefix`.

To support storing configuration data for runtime-added zones in an LMDB
database, the server must be linked with liblmdb. If this is installed in a
nonstandard location, specify the prefix using `with-lmdb=/prefix`.

To support GeoIP location-based ACLs, the server must be linked with
libGeoIP. This is not turned on by default; BIND must be configured with
`--with-geoip`. If the library is installed in a nonstandard location,
specify the prefix using `--with-geoip=/prefix`.

For DNSTAP packet logging, you must have installed libfstrm
[https://github.com/farsightsec/fstrm](https://github.com/farsightsec/fstrm)
and libprotobuf-c
[https://developers.google.com/protocol-buffers](https://developers.google.com/protocol-buffers),
and BIND must be configured with `--enable-dnstap`.

Certain compiled-in constants and default settings can be increased to
values better suited to large servers with abundant memory resources (e.g,
64-bit servers with 12G or more of memory) by specifying
`--with-tuning=large` on the `configure` command line. This can improve
performance on big servers, but will consume more memory and may degrade
performance on smaller systems.

On Linux, process capabilities are managed in user space using
the `libcap` library, which can be installed on most Linux systems via
the `libcap-dev` or `libcap-devel` module. Process capability support can
also be disabled by configuring with `--disable-linux-caps`.

On some platforms it is necessary to explicitly request large file support
to handle files bigger than 2GB.  This can be done by using
`--enable-largefile` on the `configure` command line.

Support for the "fixed" rrset-order option can be enabled or disabled by
specifying `--enable-fixed-rrset` or `--disable-fixed-rrset` on the
configure command line.  By default, fixed rrset-order is disabled to
reduce memory footprint.

The `--enable-querytrace` option causes `named` to log every step of
processing every query. This should only be enabled when debugging, because
it has a significant negative impact on query performance.

`make install` will install `named` and the various BIND 9 libraries.  By
default, installation is into /usr/local, but this can be changed with the
`--prefix` option when running `configure`.

You may specify the option `--sysconfdir` to set the directory where
configuration files like `named.conf` go by default, and `--localstatedir`
to set the default parent directory of `run/named.pid`.   For backwards
compatibility with BIND 8, `--sysconfdir` defaults to `/etc` and
`--localstatedir` defaults to `/var` if no `--prefix` option is given.  If
there is a `--prefix` option, sysconfdir defaults to `$prefix/etc` and
localstatedir defaults to `$prefix/var`.

### <a name="testing"/> Automated testing

A system test suite can be run with `make test`.  The system tests require
you to configure a set of virtual IP addresses on your system (this allows
multiple servers to run locally and communicate with one another).  These
IP addresses can be configured by running the command
`bin/tests/system/ifconfig.sh up` as root.

Some tests require Perl and the Net::DNS and/or IO::Socket::INET6 modules,
and will be skipped if these are not available. Some tests require Python
and the 'dnspython' module and will be skipped if these are not available.
See bin/tests/system/README for further details.

Unit tests are implemented using the CMocka unit testing framework.
To build them, use `configure --with-cmocka`. Execution of tests is done
by the Kyua test execution engine; if the `kyua` command is available,
then unit tests can be run via `make test` or `make unit`.

### <a name="doc"/> Documentation

The *BIND 9 Administrator Reference Manual* is included with the source
distribution, in DocBook XML, HTML and PDF format, in the `doc/arm`
directory.

Some of the programs in the BIND 9 distribution have man pages in their
directories.  In particular, the command line options of `named` are
documented in `bin/named/named.8`.

Frequently (and not-so-frequently) asked questions and their answers
can be found in the ISC Knowledge Base at
[https://kb.isc.org](https://kb.isc.org).

Additional information on various subjects can be found in other
`README` files throughout the source tree.

### <a name="changes"/> Change log

A detailed list of all changes that have been made throughout the
development BIND 9 is included in the file CHANGES, with the most recent
changes listed first.  Change notes include tags indicating the category of
the change that was made; these categories are:

|Category	|Description	        			|
|--------------	|-----------------------------------------------|
| [func] | New feature |
| [bug] | General bug fix |
| [security] | Fix for a significant security flaw |
| [experimental] | Used for new features when the syntax or other aspects of the design are still in flux and may change |
| [port] | Portability enhancement |
| [maint] | Updates to built-in data such as root server addresses and keys |
| [tuning] | Changes to built-in configuration defaults and constants to improve performance |
| [performance] | Other changes to improve server performance |
| [protocol] | Updates to the DNS protocol such as new RR types |
| [test] | Changes to the automatic tests, not affecting server functionality |
| [cleanup] | Minor corrections and refactoring |
| [doc] | Documentation |
| [contrib] | Changes to the contributed tools and libraries in the 'contrib' subdirectory |
| [placeholder] | Used in the master development branch to reserve change numbers for use in other branches, e.g. when fixing a bug that only exists in older releases |

In general, [func] and [experimental] tags will only appear in new-feature
releases (i.e., those with version numbers ending in zero).  Some new
functionality may be backported to older releases on a case-by-case basis.
All other change types may be applied to all currently-supported releases.

### <a name="ack"/> Acknowledgments

* The original development of BIND 9 was underwritten by the
  following organizations:

		Sun Microsystems, Inc.
		Hewlett Packard
		Compaq Computer Corporation
		IBM
		Process Software Corporation
		Silicon Graphics, Inc.
		Network Associates, Inc.
		U.S. Defense Information Systems Agency
		USENIX Association
		Stichting NLnet - NLnet Foundation
		Nominum, Inc.

* This product includes software developed by the OpenSSL Project for use
  in the OpenSSL Toolkit.
  [http://www.OpenSSL.org/](http://www.OpenSSL.org/)
* This product includes cryptographic software written by Eric Young
  (eay@cryptsoft.com)
* This product includes software written by Tim Hudson (tjh@cryptsoft.com)
