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

Notes for BIND 9.18.2
---------------------

New Features
~~~~~~~~~~~~

- Add a new configuration option :any:`reuseport` to disable load balancing
  on sockets in situations where processing of Response Policy Zones
  (RPZ), Catalog Zones, or large zone transfers can cause service
  disruptions. See the BIND 9 ARM for more detail. :gl:`#3249`

Bug Fixes
~~~~~~~~~

- Previously, zone maintenance DNS queries retried forever if the
  destination server was unreachable. These queries included outgoing
  NOTIFY messages, refresh SOA queries, parental DS checks, and stub
  zone NS queries. For example, if a zone had any nameservers with IPv6
  addresses and a secondary server without IPv6 connectivity, that
  server would keep trying to send a growing amount of NOTIFY traffic
  over IPv6. This futile traffic was not logged. This excessive retry
  behavior has been fixed. :gl:`#3242`

- A number of crashes and hangs which could be triggered in
  :iscman:`dig` were identified and addressed. :gl:`#3020` :gl:`#3128`
  :gl:`#3145` :gl:`#3184` :gl:`#3205` :gl:`#3244` :gl:`#3248`

- Invalid :any:`dnssec-policy` definitions, where the defined keys did not
  cover both KSK and ZSK roles for a given algorithm, were being
  accepted. These are now checked, and the :any:`dnssec-policy` is rejected
  if both roles are not present for all algorithms in use. :gl:`#3142`

- Handling of TCP write timeouts has been improved to track the timeout
  for each TCP write separately, leading to a faster connection teardown
  in case the other party is not reading the data. :gl:`#3200`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
