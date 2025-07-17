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

BIND 9.20.10
------------

New Features
~~~~~~~~~~~~

- Implement a new 'notify-defer' configuration option. ``a24db6433e6``

  This new option sets a delay (in seconds) to wait before sending a set
  of NOTIFY messages for a zone. Whenever a NOTIFY message is ready to
  be sent, sending will be deferred for this duration. This option is
  not to be confused with the :any:`notify-delay` option. The default is
  0 seconds. :gl:`#5259` :gl:`!10465`

Removed Features
~~~~~~~~~~~~~~~~

- Implement the systemd notification protocol manually to remove
  dependency on libsystemd. ``4f7e806a12b``

  libsystemd, despite being useful, adds a huge surface area for just
  using the sd_notify API. libsystemd's surface has been exploited in
  the past [1].

  Implement the systemd notification protocol by hand since it is just
  sending newline-delimited datagrams to a UNIX socket. The code
  shouldn't need more attention in the future since the notification
  protocol is covered under systemd's stability promise [2].

  We don't need to support VSOCK-backed service notifications since they
  are only intended for virtual machine inits.

  [1]: https://www.openwall.com/lists/oss-security/2024/03/29/4 [2]:
  https://systemd.io/PORTABILITY_AND_STABILITY/ :gl:`!10454`

Bug Fixes
~~~~~~~~~

- Fix zone deletion issue. ``66fc4ee86e0``

  A secondary zone could initiate a new zone transfer from the primary
  server after it had been already deleted from the secondary server,
  and before the internal garbage collection was activated to clean it
  up completely. This has been fixed. :gl:`#5291` :gl:`!10496`

- Fix a zone refresh bug. ``f09bb8b88c6``

  A secondary zone could fail to further refresh with new versions of
  the zone from a primary server if named was reconfigured during the
  SOA request step of an ongoing zone transfer. This has been fixed.
  :gl:`#5307` :gl:`!10495`

- Allow keystore.c to compile on Solaris. ``108adab25a0``

  keystore.c failed to compile on Solaris because NAME_MAX was
  undefined.  Include 'isc/dir.h' which defines NAME_MAX for platforms
  that don't define it. :gl:`#5327` :gl:`!10523`

- Set name for all the isc_mem contexts. ``bdcd698edf7``

  :gl:`!10498`


