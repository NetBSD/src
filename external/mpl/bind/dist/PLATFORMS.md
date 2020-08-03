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
system with a C11-compliant C compiler, BSD-style sockets with RFC-compliant
IPv6 support, POSIX-compliant threads, the `libuv` asynchronous I/O library,
and the OpenSSL cryptography library.

The following C11 features are used in BIND 9:

* Atomic operations support from the compiler is needed, either in the form of
  builtin operations, C11 atomics, or the `Interlocked` family of functions on
  Windows.

* Thread Local Storage support from the compiler is needed, either in the form
  of C11 `_Thread_local`/`thread_local`, the `__thread` GCC extension, or
  the `__declspec(thread)` MSVC extension on Windows.

BIND 9.16 requires a fairly recent version of `libuv` (at least 1.x).  For
some of the older systems listed below, you will have to install an updated
`libuv` package from sources such as EPEL, PPA, or other native sources for
updated packages. The other option is to build and install `libuv` from
source.

Certain optional BIND features have additional library dependencies.
These include `libxml2` and `libjson-c` for statistics, `libmaxminddb` for
geolocation, `libfstrm` and `libprotobuf-c` for DNSTAP, and `libidn2` for
internationalized domain name conversion.

ISC regularly tests BIND on many operating systems and architectures, but
lacks the resources to test all of them. Consequently, ISC is only able to
offer support on a "best effort" basis for some.

### Regularly tested platforms

As of Jul 2020, BIND 9.16 is fully supported and regularly tested on the
following systems:

* Debian 9, 10
* Ubuntu LTS 16.04, 20.04
* Fedora 32
* Red Hat Enterprise Linux / CentOS 7, 8
* FreeBSD 11.4, 12.1
* OpenBSD 6.7
* Alpine Linux 3.12

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
* NetBSD
* Other Linux distributions still supported by their vendors, such as:
    * Ubuntu 19.04+
    * Gentoo
    * Arch Linux
* OpenWRT/LEDE 17.01+
* Other CPU architectures (mips, mipsel, sparc, ...)

### Community maintained

These systems may not all have the required dependencies for building BIND
easily available, although it will be possible in many cases to compile
those directly from source. The community and interested parties may wish
to help with maintenance, and we welcome patch contributions, although we
cannot guarantee that we will accept them.  All contributions will be
assessed against the risk of adverse effect on officially supported
platforms.

* Platforms past or close to their respective EOL dates, such as:
    * Ubuntu 14.04, 18.10
    * CentOS 6
    * Debian Jessie
    * FreeBSD 10.x

## Unsupported platforms

These are platforms on which BIND 9.16 is known *not* to build or run:

* Platforms without at least OpenSSL 1.0.2
* Windows 10 / x86
* Windows Server 2012 and older
* Solaris 10 and older
* Platforms that don't support IPv6 Advanced Socket API (RFC 3542)
* Platforms that don't support atomic operations (via compiler or library)
* Linux without NPTL (Native POSIX Thread Library)
* Platforms on which `libuv` cannot be compiled
