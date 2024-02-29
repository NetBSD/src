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

Notes for BIND 9.18.12
----------------------

Removed Features
~~~~~~~~~~~~~~~~

- Specifying a ``port`` when configuring source addresses (i.e., as an
  argument to :any:`query-source`, :any:`query-source-v6`,
  :any:`transfer-source`, :any:`transfer-source-v6`,
  :any:`notify-source`, :any:`notify-source-v6`, :any:`parental-source`,
  or :any:`parental-source-v6`, or in the ``source`` or ``source-v6``
  arguments to :any:`primaries`, :any:`parental-agents`,
  :any:`also-notify`, or :any:`catalog-zones`) has been deprecated. In
  addition, the :any:`use-v4-udp-ports`, :any:`use-v6-udp-ports`,
  :any:`avoid-v4-udp-ports`, and :any:`avoid-v6-udp-ports` options have
  also been deprecated.

  Warnings are now logged when any of these options are encountered in
  ``named.conf``. In a future release, they will be made nonfunctional.
  :gl:`#3781`

Bug Fixes
~~~~~~~~~

- A constant stream of zone additions and deletions via ``rndc
  reconfig`` could cause increased memory consumption due to delayed
  cleaning of view memory. This has been fixed. :gl:`#3801`

- The speed of the message digest algorithms (MD5, SHA-1, SHA-2), and of
  NSEC3 hashing, has been improved. :gl:`#3795`

- Pointing :any:`parental-agents` to a resolver did not work because the
  RD bit was not set on DS requests. This has been fixed. :gl:`#3783`

- Building BIND 9 failed when the ``--enable-dnsrps`` switch for
  ``./configure`` was used. This has been fixed. :gl:`#3827`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
