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

Notes for BIND 9.16.1
---------------------

Known Issues
~~~~~~~~~~~~

-  UDP network ports used for listening can no longer simultaneously be
   used for sending traffic. An example configuration which triggers
   this issue would be one which uses the same address:port pair for
   ``listen-on(-v6)`` statements as for ``notify-source(-v6)`` or
   ``transfer-source(-v6)``. While this issue affects all operating
   systems, it only triggers log messages (e.g. "unable to create
   dispatch for reserved port") on some of them. There are currently no
   plans to make such a combination of settings work again.

-  See :ref:`above <relnotes_known_issues>` for a list of all known
   issues affecting this BIND 9 branch.

Feature Changes
~~~~~~~~~~~~~~~

-  The system-provided POSIX Threads read-write lock implementation is
   now used by default instead of the native BIND 9 implementation.
   Please be aware that glibc versions 2.26 through 2.29 had a
   `bug <https://sourceware.org/bugzilla/show_bug.cgi?id=23844>`__ that
   could cause BIND 9 to deadlock. A fix was released in glibc 2.30, and
   most current Linux distributions have patched or updated glibc, with
   the notable exception of Ubuntu 18.04 (Bionic) which is a work in
   progress. If you are running on an affected operating system, compile
   BIND 9 with ``--disable-pthread-rwlock`` until a fixed version of
   glibc is available. :gl:`!3125`

Bug Fixes
~~~~~~~~~

-  Fixed re-signing issues with inline zones which resulted in records
   being re-signed late or not at all.
