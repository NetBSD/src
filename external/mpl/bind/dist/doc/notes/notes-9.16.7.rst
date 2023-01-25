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

Notes for BIND 9.16.7
---------------------

New Features
~~~~~~~~~~~~

- Add a new ``rndc`` command, ``rndc dnssec -checkds``, which signals to
  ``named`` that a DS record for a given zone or key has been published
  or withdrawn from the parent. This command replaces the time-based
  ``parent-registration-delay`` configuration option. :gl:`#1613`

- Log when ``named`` adds a CDS/CDNSKEY to the zone. :gl:`#1748`

Bug Fixes
~~~~~~~~~

- In rare circumstances, ``named`` would exit with an assertion failure
  when the number of nodes stored in the red-black tree exceeded the
  maximum allowed size of the internal hash table. :gl:`#2104`

- Silence spurious system log messages for an EPROTO(71) error code that
  was seen on older operating systems, where unhandled ICMPv6 errors
  resulted in a generic protocol error being returned instead of a more
  specific error code. :gl:`#1928`

- With query name minimization enabled, ``named`` failed to resolve
  ``ip6.arpa.`` names that had extra labels to the left of the IPv6
  part. For example, when ``named`` attempted query name minimization on
  a name like ``A.B.1.2.3.4.(...).ip6.arpa.``, it stopped at the
  leftmost IPv6 label, i.e. ``1.2.3.4.(...).ip6.arpa.``, without
  considering the extra labels (``A.B``). That caused a query loop when
  resolving the name: if ``named`` received NXDOMAIN answers, then the
  same query was repeatedly sent until the number of queries sent
  reached the value of the ``max-recursion-queries`` configuration
  option. :gl:`#1847`

- Parsing of LOC records was made more strict by rejecting a sole period
  (``.``) and/or ``m`` as a value. These changes prevent zone files
  using such values from being loaded. Handling of negative altitudes
  which are not integers was also corrected. :gl:`#2074`

- Several problems found by `OSS-Fuzz`_ were fixed. (None of these are
  security issues.) :gl:`!3953` :gl:`!3975`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.

.. _OSS-Fuzz: https://github.com/google/oss-fuzz
