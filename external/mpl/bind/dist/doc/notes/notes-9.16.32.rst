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

Notes for BIND 9.16.32
----------------------

Feature Changes
~~~~~~~~~~~~~~~

- The DNSSEC algorithms RSASHA1 and NSEC3RSASHA1 are now automatically
  disabled on systems where they are disallowed by the security policy
  (e.g. Red Hat Enterprise Linux 9). Primary zones using those
  algorithms need to be migrated to new algorithms prior to running on
  these systems, as graceful migration to different DNSSEC algorithms is
  not possible when RSASHA1 is disallowed by the operating system.
  :gl:`#3469`

- Log messages related to fetch limiting have been improved to provide
  more complete information. Specifically, the final counts of allowed
  and spilled fetches are now logged before the counter object is
  destroyed. :gl:`#3461`

Bug Fixes
~~~~~~~~~

- Non-dynamic zones that inherit ``dnssec-policy`` from the
  ``view`` or ``options`` blocks were not
  marked as inline-signed and therefore never scheduled to be re-signed.
  This has been fixed. :gl:`#3438`

- The old ``max-zone-ttl`` zone option was meant to be superseded by
  the ``max-zone-ttl`` option in ``dnssec-policy``; however, the
  latter option was not fully effective. This has been corrected: zones
  no longer load if they contain TTLs greater than the limit configured
  in ``dnssec-policy``. For zones with both the old
  ``max-zone-ttl`` option and ``dnssec-policy`` configured, the
  old option is ignored, and a warning is generated. :gl:`#2918`

- ``rndc dumpdb -expired`` was fixed to include
  expired RRsets, even if ``stale-cache-enable`` is set to ``no`` and
  the cache-cleaning time window has passed. :gl:`#3462`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
