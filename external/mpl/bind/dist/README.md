<!--
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
-->
# BIND 9

### Contents

1. [Introduction](#intro)
1. [Reporting bugs and getting help](#help)
1. [Contributing to BIND](#contrib)
1. [BIND 9.16 features](#features)
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
licensed under the terms of the Mozilla Public License, version 2.0.

For a summary of features introduced in past major releases of BIND,
see the file [HISTORY](HISTORY.md).

For a detailed list of changes made throughout the history of BIND 9, see
the file [CHANGES](CHANGES). See [below](#changes) for details on the
CHANGES file format.

For up-to-date versions and release notes, see
[https://www.isc.org/download/](https://www.isc.org/download/).

For information about supported platforms, see the
["Supported Platforms"](doc/arm/platforms.rst) section in the BIND 9
Administrator Reference Manual.

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
report it. Instead, send mail to
[security-officer@isc.org](mailto:security-officer@isc.org) using our
OpenPGP key to secure your message. (Information about OpenPGP and links
to our key can be found at
[https://www.isc.org/pgpkey](https://www.isc.org/pgpkey).) Please do not
discuss the bug on any public mailing list.

For a general overview of ISC security policies, read the Knowledge Base
article at [https://kb.isc.org/docs/aa-00861](https://kb.isc.org/docs/aa-00861).

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
- General information: [CONTRIBUTING.md](CONTRIBUTING.md)
- Code of Conduct: [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)
- BIND 9 code style: [doc/dev/style.md](doc/dev/style.md)
- BIND architecture and developer guide: [doc/dev/dev.md](doc/dev/dev.md)

Patches for BIND may be submitted as
[merge requests](https://gitlab.isc.org/isc-projects/bind9/merge_requests)
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

### <a name="features"/> BIND 9.16 features

BIND 9.16 is the current stable branch of BIND 9. It includes all
changes from the 9.15 development branch, updating the previous stable
branch, 9.14. New features include:

* New `dnssec-policy` statement to configure a key and signing policy
  for zones, enabling automatic key regeneration and rollover.
* New network manager based on `libuv`.
* Added support for the new GeoIP2 geolocation API, `libmaxminddb`.
* Improved DNSSEC trust anchor configuration using the `trust-anchors`
  statement, permitting configuration of trust anchors in DS as well as
  DNSKEY format.
* YAML output for `dig`, `mdig`, and `delv`.

### <a name="build"/> Building BIND

For information about building BIND 9, see the
["Building BIND 9"](doc/arm/build.rst) section in the BIND 9
Administrator Reference Manual.

### <a name="testing"/> Automated testing

A system test suite can be run with `make test`.  The system tests require
you to configure a set of virtual IP addresses on your system (this allows
multiple servers to run locally and communicate with one another).  These
IP addresses can be configured by running the command
`bin/tests/system/ifconfig.sh up` as root.

Some tests require Perl and the `Net::DNS` and/or `IO::Socket::INET6` modules,
and will be skipped if these are not available. Some tests require Python
and the `dnspython` module and will be skipped if these are not available.
See bin/tests/system/README for further details.

Unit tests are implemented using the [CMocka unit testing framework](https://cmocka.org/).
To build them, use `configure --with-cmocka`. Execution of tests is done
by the [Kyua test execution engine](https://github.com/jmmv/kyua); if the
`kyua` command is available, then unit tests can be run via `make test`
or `make unit`.

### <a name="doc"/> Documentation

The *BIND 9 Administrator Reference Manual* is included with the source
distribution, in DocBook XML, HTML, and PDF format, in the `doc/arm`
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
| [placeholder] | Used in the main development branch to reserve change numbers for use in other branches, e.g., when fixing a bug that only exists in older releases |

In general, [func] and [experimental] tags will only appear in new-feature
releases (i.e., those with version numbers ending in zero).  Some new
functionality may be backported to older releases on a case-by-case basis.
All other change types may be applied to all currently-supported releases.

#### Bug report identifiers

Most notes in the CHANGES file include a reference to a bug report or
issue number. Prior to 2018, these were usually of the form `[RT #NNN]`
and referred to entries in the "bind9-bugs" RT database, which was not open
to the public. More recent entries use the form `[GL #NNN]` or, less often,
`[GL !NNN]`, which, respectively, refer to issues or merge requests in the
GitLab database. Most of these are publicly readable, unless they include
information which is confidential or security sensitive.

To look up a GitLab issue by its number, use the URL
[https://gitlab.isc.org/isc-projects/bind9/issues/NNN](https://gitlab.isc.org/isc-projects/bind9/issues).
To look up a merge request, use
[https://gitlab.isc.org/isc-projects/bind9/merge_requests/NNN](https://gitlab.isc.org/isc-projects/bind9/merge_requests).

In rare cases, an issue or merge request number may be followed with the
letter "P". This indicates that the information is in the private ISC
GitLab instance, which is not visible to the public.

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
