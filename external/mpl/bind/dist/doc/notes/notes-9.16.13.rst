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

Notes for BIND 9.16.13
----------------------

New Features
~~~~~~~~~~~~

- A new ``purge-keys`` option has been added to ``dnssec-policy``. It
  sets the period of time that key files are retained after becoming
  obsolete due to a key rollover; the default is 90 days. This feature
  can be disabled by setting ``purge-keys`` to 0. :gl:`#2408`

Feature Changes
~~~~~~~~~~~~~~~

- When serve-stale is enabled and stale data is available, ``named`` now
  returns stale answers upon encountering any unexpected error in the
  query resolution process. This may happen, for example, if the
  ``fetches-per-server`` or ``fetches-per-zone`` limits are reached. In
  this case, ``named`` attempts to answer DNS requests with stale data,
  but does not start the ``stale-refresh-time`` window. :gl:`#2434`

Bug Fixes
~~~~~~~~~

- Zone journal (``.jnl``) files created by versions of ``named`` prior
  to 9.16.12 were no longer compatible; this could cause problems when
  upgrading if journal files were not synchronized first. This has been
  corrected: older journal files can now be read when starting up. When
  an old-style journal file is detected, it is updated to the new format
  immediately after loading.

  Note that journals created by the current version of ``named`` are not
  usable by versions prior to 9.16.12. Before downgrading to a prior
  release, users are advised to ensure that all dynamic zones have been
  synchronized using ``rndc sync -clean``.

  A journal file's format can be changed manually by running
  ``named-journalprint -d`` (downgrade) or ``named-journalprint -u``
  (upgrade). Note that this *must not* be done while ``named`` is
  running. :gl:`#2505`

- ``named`` crashed when it was allowed to serve stale answers and
  ``stale-answer-client-timeout`` was triggered without any (stale) data
  available in the cache to answer the query. :gl:`#2503`

- If an outgoing packet exceeded ``max-udp-size``, ``named`` dropped it
  instead of sending back a proper response. To prevent this problem,
  the ``IP_DONTFRAG`` option is no longer set on UDP sockets, which has
  been happening since BIND 9.16.11. :gl:`#2466`

- NSEC3 records were not immediately created when signing a dynamic zone
  using ``dnssec-policy`` with ``nsec3param``. This has been fixed.
  :gl:`#2498`

- A memory leak occurred when ``named`` was reconfigured after adding an
  inline-signed zone with ``auto-dnssec maintain`` enabled. This has
  been fixed. :gl:`#2041`

- An invalid direction field (not one of ``N``, ``S``, ``E``, ``W``) in
  a LOC record resulted in an INSIST failure when a zone file containing
  such a record was loaded. :gl:`#2499`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
