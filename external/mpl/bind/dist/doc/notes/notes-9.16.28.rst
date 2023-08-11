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

Notes for BIND 9.16.28
----------------------

New Features
~~~~~~~~~~~~

- Add a new configuration option ``reuseport`` to disable load balancing
  on sockets in situations where processing of Response Policy Zones
  (RPZ), Catalog Zones, or large zone transfers can cause service
  disruptions. See the BIND 9 ARM for more detail. :gl:`#3249`

Bug Fixes
~~~~~~~~~

- Invalid ``dnssec-policy`` definitions, where the defined keys did not
  cover both KSK and ZSK roles for a given algorithm, were being
  accepted. These are now checked, and the ``dnssec-policy`` is rejected
  if both roles are not present for all algorithms in use. :gl:`#3142`

- Handling of TCP write timeouts has been improved to track the timeout
  for each TCP write separately, leading to a faster connection teardown
  in case the other party is not reading the data. :gl:`#3200`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
