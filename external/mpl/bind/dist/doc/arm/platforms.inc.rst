.. Copyright (C) Internet Systems Consortium, Inc. ("ISC")
..
.. SPDX-License-Identifier: MPL-2.0
..
.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0.  If a copy of the MPL was not distributed with this
.. file, you can obtain one at https://mozilla.org/MPL/2.0/.
..
.. See the COPYRIGHT file distributed with this work for additional
.. information regarding copyright ownership.

.. _supported_os:

Supported Platforms
-------------------

The current support status of BIND 9 versions across various platforms can be
found in the ISC Knowledgebase:

https://kb.isc.org/docs/supported-platforms

In general, this version of BIND will build and run on any
POSIX-compliant system with a modern C11 (or better) compiler, BSD-style
sockets with RFC-compliant IPv6 support, POSIX-compliant threads, and
the :ref:`required libraries <build_dependencies>`.

The following C11 features are required to compile BIND 9:

-  Atomic operations support defined in <stdatomic.h>

-  Thread Local Storage support defined in <threads.h>

Where it makes sense, BIND 9 uses C-standard fixes introduced by the C17 update
of the C11 standard.

ISC regularly tests BIND on many operating systems and architectures,
but lacks the resources to test all of them. Consequently, ISC is only
able to offer support on a “best-effort” basis for some.

Regularly Tested Platforms
~~~~~~~~~~~~~~~~~~~~~~~~~~

Current versions of BIND 9 are fully supported and regularly tested on the
following systems:

-  Debian 12, 13
-  Ubuntu LTS 22.04, 24.04
-  Fedora 43
-  Red Hat Enterprise Linux / CentOS / AlmaLinux 8, 9, 10
-  FreeBSD 13, 14, 15
-  Alpine Linux 3.23

The amd64 CPU architecture is fully supported and regularly tested.

Best-Effort
~~~~~~~~~~~

The following are platforms on which BIND is known to build and run. ISC
makes every effort to fix bugs on these platforms, but may be unable to
do so quickly due to lack of hardware, less familiarity on the part of
engineering staff, and other constraints. None of these are tested
regularly by ISC.

-  macOS 10.12+
-  Solaris 11
-  NetBSD
-  OpenBSD
-  Other Linux distributions still supported by their vendors, such as:

   -  Ubuntu 22.10+
   -  Gentoo
   -  Arch Linux

-  OpenWRT/LEDE 17.01+
-  Other CPU architectures (arm, arm64, mips64, ppc64, s390x)

Community-Maintained
~~~~~~~~~~~~~~~~~~~~

These systems may not all have the required dependencies for building
BIND easily available, although it is possible in many cases to
compile those directly from source. The community and interested parties
may wish to help with maintenance, and we welcome patch contributions,
although we cannot guarantee that we will accept them. All contributions
will be assessed against the risk of adverse effect on officially
supported platforms.

-  Platforms past or close to their respective EOL dates, such as:

   -  Ubuntu 14.04, 16.04, 18.04, 20.04 (Ubuntu ESM releases are not supported)
   -  Red Hat Enterprise Linux / CentOS / Oracle Linux 6, 7
   -  Debian 8 Jessie, 9 Stretch, 10 Buster, 11 Bullseye
   -  FreeBSD 10.x, 11.x, 12.x

-  Less common CPU architectures (i386, i686, mips, mipsel, sparc, ppc, and others)

Unsupported Platforms
---------------------

These are platforms on which current versions of BIND 9 are known *not* to build or run:

-  Platforms without at least OpenSSL 1.0.2
-  Windows
-  Solaris 10 and older
-  Platforms that do not support IPv6 Advanced Socket API (:rfc:`3542`)
-  Platforms that do not support atomic operations (via compiler or
   library)
-  Linux without NPTL (Native POSIX Thread Library)
-  Platforms on which **libuv >= 1.34** cannot be compiled or is not available

Installing BIND 9
-----------------

:ref:`build_bind` contains complete instructions for how to build BIND 9.

The ISC `Knowledgebase <https://kb.isc.org/>`_ contains many useful articles about installing
BIND 9 on specific platforms.

