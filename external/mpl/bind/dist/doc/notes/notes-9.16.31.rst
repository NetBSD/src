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

Notes for BIND 9.16.31
----------------------

Bug Fixes
~~~~~~~~~

- An assertion failure caused by a TCP connection closing between a
  connect (or accept) and a read from a socket has been fixed.
  :gl:`#3400`

- :iscman:`named` could crash during a very rare situation that could
  arise when validating a query which had timed out at that exact
  moment. This has been fixed. :gl:`#3398`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
