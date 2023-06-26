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

Notes for BIND 9.16.39
----------------------

Feature Changes
~~~~~~~~~~~~~~~

- libuv support for receiving multiple UDP messages in a single
  ``recvmmsg()`` system call has been tweaked several times between
  libuv versions 1.35.0 and 1.40.0; the current recommended libuv
  version is 1.40.0 or higher. New rules are now in effect for running
  with a different version of libuv than the one used at compilation
  time. These rules may trigger a fatal error at startup:

  - Building against or running with libuv versions 1.35.0 and 1.36.0 is
    now a fatal error.

  - Running with libuv version higher than 1.34.2 is now a fatal error
    when :iscman:`named` is built against libuv version 1.34.2 or lower.

  - Running with libuv version higher than 1.39.0 is now a fatal error
    when :iscman:`named` is built against libuv version 1.37.0, 1.38.0,
    1.38.1, or 1.39.0.

  This prevents the use of libuv versions that may trigger an assertion
  failure when receiving multiple UDP messages in a single system call.
  :gl:`#3840`

Bug Fixes
~~~~~~~~~

- :iscman:`named` could crash with an assertion failure when adding a
  new zone into the configuration file for a name which was already
  configured as a member zone for a catalog zone. This has been fixed.
  :gl:`#3911`

- When :iscman:`named` starts up, it sends a query for the DNSSEC key
  for each configured trust anchor to determine whether the key has
  changed. In some unusual cases, the query might depend on a zone for
  which the server is itself authoritative, and would have failed if it
  were sent before the zone was fully loaded. This has now been fixed by
  delaying the key queries until all zones have finished loading.
  :gl:`#3673`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
