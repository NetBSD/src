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
## Supported platforms

In general, this version of BIND will build and run on any POSIX-compliant
system with a C99-compliant C compiler, BSD-style sockets with RFC-compliant
IPv6 support, POSIX-compliant threads, and the OpenSSL cryptography library.
Atomic operations support from the compiler is needed, either in the form of
builtin operations, C11 atomics or the Interlocked family of functions on
Windows.

ISC regularly tests BIND on many operating systems and architectures, but
lacks the resources to test all of them. Consequently, ISC is only able to
offer support on a "best effort" basis for some.

### Regularly tested platforms

As of Feb 2019, BIND 9.14 is fully supported and regularly tested on the
following systems:

* Debian 8, 9, 10
* Ubuntu 16.04, 18.04
* Fedora 28, 29
* Red Hat Enterprise Linux / CentOS 6, 7
* FreeBSD 11.x
* OpenBSD 6.2, 6.3

The amd64, i386, armhf and arm64 CPU architectures are all fully supported.

### Best effort

The following are platforms on which BIND is known to build and run.
ISC makes every effort to fix bugs on these platforms, but may be unable to
do so quickly due to lack of hardware, less familiarity on the part of
engineering staff, and other constraints. With the exception of Windows
Server 2012 R2, none of these are tested regularly by ISC.

* Windows Server 2012 R2, 2016 / x64
* Windows 10 / x64
* macOS 10.12+
* Solaris 11
* FreeBSD 10.x, 12.0+
* OpenBSD 6.4+
* NetBSD
* Other Linux distributions still supported by their vendors, such as:
    * Ubuntu 14.04, 18.10+
    * Gentoo
    * Arch Linux
    * Alpine Linux
* OpenWRT/LEDE 17.01+
* Other CPU architectures (mips, mipsel, sparc, ...)

## Unsupported platforms

These are platforms on which BIND 9.14 is known *not* to build or run:

* Platforms without at least OpenSSL 1.0.2
* Windows 10 / x86
* Windows Server 2012 and older
* Solaris 10 and older
* Platforms that don't support IPv6 Advanced Socket API (RFC 3542)
* Platforms that don't support atomic operations (via compiler or library)
* Linux without NPTL (Native POSIX Thread Library)

## Platform quirks

### NetBSD 6 i386

The i386 build of NetBSD requires the `libatomic` library, available from
the `gcc5-libs` package.  Because this library is in a non-standard path,
its location must be specified in the `configure` command line:

```
LDFLAGS="-L/usr/pkg/gcc5/i486--netbsdelf/lib/ -Wl,-R/usr/pkg/gcc5/i486--netbsdelf/lib/" ./configure
```
