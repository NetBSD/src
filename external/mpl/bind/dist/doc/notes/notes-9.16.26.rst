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

Notes for BIND 9.16.26
----------------------

Feature Changes
~~~~~~~~~~~~~~~

- The DLZ API has been updated: EDNS Client-Subnet (ECS) options sent
  by a client are now included in the client information sent to DLZ
  modules when processing queries. :gl:`#3082`

Bug Fixes
~~~~~~~~~

- Previously, ``recvmmsg`` support was enabled in libuv 1.35.0 and
  1.36.0, but not in libuv versions 1.37.0 or greater, reducing the
  maximum query-response performance. This has been fixed. :gl:`#3095`

- A failed view configuration during a ``named`` reconfiguration
  procedure could cause inconsistencies in BIND internal structures,
  causing a crash or other unexpected errors. This has been fixed.
  :gl:`#3060`

- Previously, ``named`` logged a "quota reached" message when it hit its
  hard quota on the number of connections. That message was accidentally
  removed but has now been restored. :gl:`#3125`

- Build errors were introduced in some DLZ modules due to an incomplete
  change in the previous release. This has been fixed. :gl:`#3111`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
