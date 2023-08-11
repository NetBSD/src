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

Notes for BIND 9.16.10
----------------------

New Features
~~~~~~~~~~~~

- NSEC3 support was added to KASP. A new option for ``dnssec-policy``,
  ``nsec3param``, can be used to set the desired NSEC3 parameters.
  NSEC3 salt collisions are automatically prevented during resalting.
  :gl:`#1620`

Feature Changes
~~~~~~~~~~~~~~~

- The default value of ``max-recursion-queries`` was increased from 75
  to 100. Since the queries sent towards root and TLD servers are now
  included in the count (as a result of the fix for CVE-2020-8616),
  ``max-recursion-queries`` has a higher chance of being exceeded by
  non-attack queries, which is the main reason for increasing its
  default value. :gl:`#2305`

- The default value of ``nocookie-udp-size`` was restored back to 4096
  bytes. Since ``max-udp-size`` is the upper bound for
  ``nocookie-udp-size``, this change relieves the operator from having
  to change ``nocookie-udp-size`` together with ``max-udp-size`` in
  order to increase the default EDNS buffer size limit.
  ``nocookie-udp-size`` can still be set to a value lower than
  ``max-udp-size``, if desired. :gl:`#2250`

Bug Fixes
~~~~~~~~~

- Handling of missing DNS COOKIE responses over UDP was tightened by
  falling back to TCP. :gl:`#2275`

- The CNAME synthesized from a DNAME was incorrectly followed when the
  QTYPE was CNAME or ANY. :gl:`#2280`

- Building with native PKCS#11 support for AEP Keyper has been broken
  since BIND 9.16.6. This has been fixed. :gl:`#2315`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
