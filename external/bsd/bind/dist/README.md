<!--
 - Copyright (C) 2017  Internet Systems Consortium, Inc. ("ISC")
 -
 - Permission to use, copy, modify, and/or distribute this software for any
 - purpose with or without fee is hereby granted, provided that the above
 - copyright notice and this permission notice appear in all copies.
 -
 - THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 - REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 - AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 - INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 - LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 - OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 - PERFORMANCE OF THIS SOFTWARE.
-->
# BIND 9

### Contents

1. [Introduction](#intro)
1. [Reporting bugs and getting help](#help)
1. [Contributing to BIND](#contrib)
1. [BIND 9.9 features](#features)
1. [Building BIND](#build)
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

BIND 9 is a complete re-write of the BIND architecture that was used in
versions 4 and 8.  Internet Systems Consortium
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

### <a name="help"/> Reporting bugs and getting help

Please report assertion failure errors and suspected security issues to
[security-officer@isc.org](mailto:security-officer@isc.org).

General bug reports can be sent to
[bind9-bugs@isc.org](mailto:bind9-bugs@isc.org).

Feature requests can be sent to
[bind-suggest@isc.org](mailto:bind-suggest@isc.org).

Please note that, while ISC's ticketing system is not currently publicly
readable, this may change in the future.  Please do not include information
in bug reports that you consider to be confidential. For example, when
sending the contents of your configuration file, it is advisable to obscure
key secrets; this can be done automatically by using `named-checkconf -px`.

Professional support and training for BIND are available from
ISC at [https://www.isc.org/support](https://www.isc.org/support).

To join the __BIND Users__ mailing list, or view the archives, visit
[https://lists.isc.org/mailman/listinfo/bind-users](https://lists.isc.org/mailman/listinfo/bind-users).

If you're planning on making changes to the BIND 9 source code, you
may also want to join the __BIND Workers__ mailing list, at
[https://lists.isc.org/mailman/listinfo/bind-workers](https://lists.isc.org/mailman/listinfo/bind-workers).

### <a name="contrib"/> Contributing to BIND

A public git repository for BIND is maintained at
[http://www.isc.org/git/](http://www.isc.org/git/), and also on Github
at [https://github.com/isc-projects](https://github.com/isc-projects).

Information for BIND contributors can be found in the following files:
- General information: [doc/dev/contrib.md](doc/dev/contrib.md)
- BIND 9 code style: [doc/dev/style.md](doc/dev/style.md)
- BIND architecture and developer guide: [doc/dev/dev.md](doc/dev/dev.md)

Patches for BIND may be submitted either as Github pull requests
or via email.  When submitting a patch via email, please prepend the
subject header with "`[PATCH]`" so it will be easier for us to find. 
If your patch introduces a new feature in BIND, please submit it to
[bind-suggest@isc.org](mailto:bind-suggest@isc.org); if it fixes a bug,
please submit it to [bind9-bugs@isc.org](mailto:bind9-bugs@isc.org).

### <a name="features"/> BIND 9.9 features

BIND 9.9.0 includes a number of changes from BIND 9.8 and earlier
releases.  New features include:

* Inline signing, allowing automatic DNSSEC signing of master zones without
  modification of the zonefile, or "bump in the wire" signing in slaves.
* NXDOMAIN redirection.
* New `rndc flushtree` command clears all data under a given name from the
  DNS cache.
* New `rndc sync` command dumps pending changes in a dynamic zone to disk
  without a freeze/thaw cycle.
* New `rndc signing` command displays or clears signing status records in
  `auto-dnssec` zones.
* NSEC3 parameters for `auto-dnssec` zones can now be set prior to signing,
  eliminating the need to initially sign with NSEC.
* Startup time improvements on large authoritative servers.
* Slave zones are now saved in raw format by default.
* Several improvements to response policy zones (RPZ).
* Improved hardware scalability by using multiple threads to listen for
  queries and using finer-grained client locking
* The `also-notify` option now takes the same syntax as `masters`, so it
  can used named masterlists and TSIG keys.
* `dnssec-signzone -D` writes an output file containing only DNSSEC data,
  which can be included by the primary zone file.
* `dnssec-signzone -R` forces removal of signatures that are not expired
  but were created by a key which no longer exists.
* `dnssec-signzone -X` allows a separate expiration date to be specified
  for DNSKEY signatures from other signatures.
* New `-L` option to `dnssec-keygen`, `dnssec-settime`, and
  `dnssec-keyfromlabel` sets the default TTL for the key.
* `dnssec-dsfromkey` now supports reading from standard input, to make it
  easier to convert DNSKEY to DS.
* RFC 1918 reverse zones have been added to the empty-zones table per RFC
  6303.
* Dynamic updates can now optionally set the zone's SOA serial number to
  the current UNIX time.
* DLZ modules can now retrieve the source IP address of the querying
  client.
* `request-ixfr` option can now be set at the per-zone level.
* `dig +rrcomments` turns on comments about DNSKEY records, indicating
  their key ID, algorithm and function
* Simplified nsupdate syntax and added readline support

#### BIND 9.9.1

BIND 9.9.1 is a maintenance release.

#### BIND 9.9.2

BIND 9.9.2 is a maintenance release, and addresses the security flaw
described in CVE-2012-4244.

#### BIND 9.9.3

BIND 9.9.3 is a maintenance release and addresses the security
flaws described in CVE-2012-5688, CVE-2012-5689 and CVE-2013-2266.

#### BIND 9.9.4

BIND 9.9.4 is a maintenance release, and addresses the security
flaws described in CVE-2013-3919 and CVE-2013-4854. It also
introduces DNS Response Rate Limiting (DNS RRL) as a
compile-time option. To use this feature, configure with
the `--enable-rrl` option.

#### BIND 9.9.5

BIND 9.9.5 is a maintenance release, and addresses the security
flaws described in CVE-2013-6320 and CVE-2014-0591.  It also
includes the following functional enhancements:

* `named` now preserves the capitalization of names when responding to
  queries.
* new `dnssec-importkey` command allows the use of offline DNSSEC keys with
  automatic DNSKEY management.
* When re-signing a zone, the new `dnssec-signzone -Q` option drops
  signatures from keys that are still published but are no longer active.
* `named-checkconf -px` will print the contents of configuration files with
  the shared secrets obscured, making it easier to share configuration
  (e.g. when submitting a bug report) without revealing private
  information.

#### BIND 9.9.6

BIND 9.9.6 is a maintenance release, and also includes the following new
functionality.

- The former behavior with respect to capitalization of names (prior to
  BIND 9.9.5) can be restored for specific clients via the new
  `no-case-compress` ACL.

#### BIND 9.9.7

BIND 9.9.7 is a maintenance release, and addresses the security flaws
described in CVE-2014-8500 and CVE-2015-1349.

#### BIND 9.9.8

BIND 9.9.8 is a maintenance release, and addresses the security flaws
described in CVE-2015-4620, CVE-2015-5477, CVE-2015-5722, and
CVE-2015-5986.

It also makes the following new features available via a compile-time
option:

* New "fetchlimit" quotas are now available for the use of
  recursive resolvers that are are under high query load for
  domains whose authoritative servers are nonresponsive or are
  experiencing a denial of service attack.
    * `fetches-per-server` limits the number of simultaneous queries that
      can be sent to any single authoritative server.  The configured value
      is a starting point; it is automatically adjusted downward if the
      server is partially or completely non-responsive. The algorithm used
      to adjust the quota can be configured via the `fetch-quota-params`
      option.
    * `fetches-per-zone` limits the number of simultaneous queries that can
      be sent for names within a single domain.  (Note: Unlike
      `fetches-per-server`, this value is not self-tuning.)
    * New stats counters have been added to count queries spilled due to
      these quotas.
  NOTE: These options are NOT built in by default; use
  `configure --enable-fetchlimit` to enable them.

#### BIND 9.9.9

BIND 9.9.9 is a maintenance release and addresses bugs found
in BIND 9.9.8 and earlier, as well as the security flaws
described in CVE-2015-8000, CVE-2015-8461, CVE-2015-8704,
CVE-2016-1285, CVE-2016-1286, CVE-2016-2775 and CVE-2016-2776.

#### BIND 9.9.10
	
BIND 9.9.10 is a maintenance release and addresses the security
flaws disclosed in CVE-2016-2775, CVE-2016-2776, CVE-2016-6170,
CVE-2016-8864, CVE-2016-9131, CVE-2016-9147, CVE-2016-9444,
CVE-2017-3135, CVE-2017-3136, CVE-2017-3137, and CVE-2017-3138.

#### BIND 9.9.11

BIND 9.9.11 is a maintenance release, and addresses the security flaws
disclosed in CVE-2017-3140, CVE-2017-3141, CVE-2017-3142 and CVE-2017-3143.

### <a name="build"/> Building BIND

BIND requires a UNIX or Linux system with an ANSI C compiler, basic POSIX
support, and a 64-bit integer type. Successful builds have been observed on
many versions of Linux and UNIX, including RedHat, Fedora, Debian, Ubuntu,
SuSE, Slackware, FreeBSD, NetBSD, OpenBSD, Mac OS X, Solaris, HP-UX, AIX,
SCO OpenServer, and OpenWRT. 

BIND is also available for Windows XP, 2003, 2008, and higher.  See
`win32utils/readme1st.txt` for details on building for Windows systems.

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

#### <a name="opts"/> Compile-time options

To see a full list of configuration options, run `configure --help`.

On most platforms, BIND 9 is built with multithreading support, allowing it
to take advantage of multiple CPUs.  You can configure this by specifying
`--enable-threads` or `--disable-threads` on the `configure` command line.
The default is to enable threads, except on some older operating systems on
which threads are known to have had problems in the past.  (Note: Prior to
BIND 9.10, the default was to disable threads on Linux systems; this has
now been reversed.  On Linux systems, the threaded build is known to change
BIND's behavior with respect to file permissions; it may be necessary to
specify a user with the -u option when running `named`.)

To build shared libraries, specify `--with-libtool` on the `configure`
command line.

Certain compiled-in constants and default settings can be increased to
values better suited to large servers with abundant memory resources (e.g,
64-bit servers with 12G or more of memory) by specifying
`--with-tuning=large` on the `configure` command line. This can improve
performance on big servers, but will consume more memory and may degrade
performance on smaller systems.

For the server to support DNSSEC, you need to build it with crypto support.
To use OpenSSL, you should have OpenSSL 1.0.2e or newer installed.  If the
OpenSSL library is installed in a nonstandard location, specify the prefix
using "--with-openssl=/prefix" on the configure command line. To use a
PKCS#11 hardware service module for cryptographic operations, specify the
path to the PKCS#11 provider library using "--with-pkcs11=/prefix", and
configure BIND with "--enable-native-pkcs11".

To support the HTTP statistics channel, the server must be linked with
libxml2 [http://xmlsoft.org](http://xmlsoft.org) If this is installed at a
nonstandard location, specify the prefix using `--with-libxml2=/prefix`.

Python requires the 'argparse' module to be available.
'argparse' is a standard module as of Python 2.7 and Python 3.2.

On some platforms it is necessary to explicitly request large file support
to handle files bigger than 2GB.  This can be done by using
`--enable-largefile` on the `configure` command line.

Support for the "fixed" rrset-order option can be enabled or disabled by
specifying `--enable-fixed-rrset` or `--disable-fixed-rrset` on the
configure command line.  By default, fixed rrset-order is disabled to
reduce memory footprint.

If your operating system has integrated support for IPv6, it will be used
automatically.  If you have installed KAME IPv6 separately, use
`--with-kame[=PATH]` to specify its location.

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
IP addresses can be configured by by running the script
`bin/tests/system/ifconfig.sh up` as root.

Some tests require Perl and the Net::DNS and/or IO::Socket::INET6 modules,
and will be skipped if these are not available. Some tests require Python
and the 'dnspython' module and will be skipped if these are not available.
See bin/tests/system/README for further details.

Unit tests are implemented using Automated Testing Framework (ATF).
To run them, use `configure --with-atf`, then run `make test` or
`make unit`.

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
