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

Notes for BIND 9.16.21
----------------------

New Features
~~~~~~~~~~~~

- Support for HTTPS and SVCB record types has been added. (This does not
  include ADDITIONAL section processing for these record types, only
  basic support for RR type parsing and printing.) :gl:`#1132`

Feature Changes
~~~~~~~~~~~~~~~

- When ``dnssec-signzone`` signs a zone using a successor key whose
  predecessor is still published, it now only refreshes signatures for
  RRsets which have an invalid signature, an expired signature, or a
  signature which expires within the provided cycle interval. This
  allows ``dnssec-signzone`` to gradually replace signatures in a zone
  whose ZSK is being rolled over (similarly to what ``auto-dnssec
  maintain;`` does). :gl:`#1551`

Bug Fixes
~~~~~~~~~

- A recent change to the internal memory structure of zone databases
  inadvertently neglected to update the MAPAPI value for zone files in
  ``map`` format. This caused version 9.16.20 of ``named`` to attempt to
  load files into memory that were no longer compatible, triggering an
  assertion failure on startup. The MAPAPI value has now been updated,
  so ``named`` rejects outdated files when encountering them.
  :gl:`#2872`

- Zone files in ``map`` format whose size exceeded 2 GB failed to load.
  This has been fixed. :gl:`#2878`

- ``named`` was unable to run as a Windows Service under certain
  circumstances. This has been fixed. :gl:`#2837`

- Stale data in the cache could cause ``named`` to send non-minimized
  queries despite QNAME minimization being enabled. This has been fixed.
  :gl:`#2665`

- When a DNSSEC-signed zone which only has a single signing key
  available is migrated to ``dnssec-policy``, that key is now treated as
  a Combined Signing Key (CSK). :gl:`#2857`

- When a dynamic zone was made available in another view using the
  ``in-view`` statement, running ``rndc freeze`` always reported an
  ``already frozen`` error even though the zone was successfully
  frozen. This has been fixed. :gl:`#2844`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
