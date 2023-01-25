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

Notes for BIND 9.16.20
----------------------

Security Fixes
~~~~~~~~~~~~~~

- Fixed an assertion failure that occurred in ``named`` when it
  attempted to send a UDP packet that exceeded the MTU size, if
  Response Rate Limiting (RRL) was enabled. (CVE-2021-25218) :gl:`#2856`

- ``named`` failed to check the opcode of responses when performing zone
  refreshes, stub zone updates, and UPDATE forwarding. This could lead
  to an assertion failure under certain conditions and has been
  addressed by rejecting responses whose opcode does not match the
  expected value. :gl:`#2762`

Feature Changes
~~~~~~~~~~~~~~~

- Testing revealed that setting the thread affinity for various types of
  ``named`` threads led to inconsistent recursive performance, as
  sometimes multiple sets of threads competed over a single resource.

  Due to the above, ``named`` no longer sets thread affinity. This
  causes a slight dip of around 5% in authoritative performance, but
  recursive performance is now consistently improved. :gl:`#2822`

- CDS and CDNSKEY records can now be published in a zone without the
  requirement that they exactly match an existing DNSKEY record, as long
  as the zone is signed with an algorithm represented in the CDS or
  CDNSKEY record. This allows a clean rollover from one DNS provider to
  another when using a multiple-signer DNSSEC configuration. :gl:`#2710`

Bug Fixes
~~~~~~~~~

- Authentication of ``rndc`` messages could fail if a ``controls``
  statement was configured with multiple key algorithms for the same
  listener. This has been fixed. :gl:`#2756`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
