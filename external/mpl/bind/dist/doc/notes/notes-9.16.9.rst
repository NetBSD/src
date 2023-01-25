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

Notes for BIND 9.16.9
---------------------

New Features
~~~~~~~~~~~~

- A new configuration option, ``stale-refresh-time``, has been
  introduced. It allows a stale RRset to be served directly from cache
  for a period of time after a failed lookup, before a new attempt to
  refresh it is made. :gl:`#2066`

Bug Fixes
~~~~~~~~~

- ``named`` could crash with an assertion failure if a TCP connection
  were closed while a request was still being processed. :gl:`#2227`

- ``named`` acting as a resolver could incorrectly treat signed zones
  with no DS record at the parent as bogus. Such zones should be treated
  as insecure. This has been fixed. :gl:`#2236`

- After a Negative Trust Anchor (NTA) is added, BIND performs periodic
  checks to see if it is still necessary. If BIND encountered a failure
  while creating a query to perform such a check, it attempted to
  dereference a ``NULL`` pointer, resulting in a crash. :gl:`#2244`

- A problem obtaining glue records could prevent a stub zone from
  functioning properly, if the authoritative server for the zone were
  configured for minimal responses. :gl:`#1736`

- ``UV_EOF`` is no longer treated as a ``TCP4RecvErr`` or a
  ``TCP6RecvErr``. :gl:`#2208`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
