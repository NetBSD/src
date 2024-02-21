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

Notes for BIND 9.18.11
----------------------

Security Fixes
~~~~~~~~~~~~~~

- An UPDATE message flood could cause :iscman:`named` to exhaust all
  available memory. This flaw was addressed by adding a new
  :any:`update-quota` option that controls the maximum number of
  outstanding DNS UPDATE messages that :iscman:`named` can hold in a
  queue at any given time (default: 100). :cve:`2022-3094`

  ISC would like to thank Rob Schulhof from Infoblox for bringing this
  vulnerability to our attention. :gl:`#3523`

- :iscman:`named` could crash with an assertion failure when an RRSIG
  query was received and :any:`stale-answer-client-timeout` was set to a
  non-zero value. This has been fixed. :cve:`2022-3736`

  ISC would like to thank Borja Marcos from Sarenet (with assistance by
  Iratxe Niño from Fundación Sarenet) for bringing this vulnerability to
  our attention. :gl:`#3622`

- :iscman:`named` running as a resolver with the
  :any:`stale-answer-client-timeout` option set to any value greater
  than ``0`` could crash with an assertion failure, when the
  :any:`recursive-clients` soft quota was reached. This has been fixed.
  :cve:`2022-3924`

  ISC would like to thank Maksym Odinintsev from AWS for bringing this
  vulnerability to our attention. :gl:`#3619`

New Features
~~~~~~~~~~~~

- The new :any:`update-quota` option can be used to control the number
  of simultaneous DNS UPDATE messages that can be processed to update an
  authoritative zone on a primary server, or forwarded to the primary
  server by a secondary server. The default is 100. A new statistics
  counter has also been added to record events when this quota is
  exceeded, and the version numbers for the XML and JSON statistics
  schemas have been updated. :gl:`#3523`

Removed Features
~~~~~~~~~~~~~~~~

- The Differentiated Services Code Point (DSCP) feature in BIND has been
  non-operational since the new Network Manager was introduced in BIND
  9.16. It is now marked as obsolete, and vestigial code implementing it
  has been removed. Configuring DSCP values in ``named.conf`` now causes
  a warning to be logged. :gl:`#3773`

Feature Changes
~~~~~~~~~~~~~~~

- The catalog zone implementation has been optimized to work with
  hundreds of thousands of member zones. :gl:`#3212` :gl:`#3744`

Bug Fixes
~~~~~~~~~

- A rare assertion failure was fixed in outgoing TCP DNS connection
  handling. :gl:`#3178` :gl:`#3636`

- Large zone transfers over TLS (XoT) could fail. This has been fixed.
  :gl:`#3772`

- In addition to a previously fixed bug, another similar issue was
  discovered where quotas could be erroneously reached for servers,
  including any configured forwarders, resulting in SERVFAIL answers
  being sent to clients. This has been fixed. :gl:`#3752`

- In certain query resolution scenarios (e.g. when following CNAME
  records), :iscman:`named` configured to answer from stale cache could
  return a SERVFAIL response despite a usable, non-stale answer being
  present in the cache. This has been fixed. :gl:`#3678`

- When an outgoing request timed out, :iscman:`named` would retry up to
  three times with the same server instead of trying the next available
  name server. This has been fixed. :gl:`#3637`

- Recently used ADB names and ADB entries (IP addresses) could get
  cleaned when ADB was under memory pressure. To mitigate this, only
  actual ADB names and ADB entries are now counted (excluding internal
  memory structures used for "housekeeping") and recently used (<= 10
  seconds) ADB names and entries are excluded from the overmem memory
  cleaner. :gl:`#3739`

- The "Prohibited" Extended DNS Error was inadvertently set in some
  NOERROR responses. This has been fixed. :gl:`#3743`

- Previously, TLS session resumption could have led to handshake
  failures when client certificates were used for authentication (Mutual
  TLS). This has been fixed. :gl:`#3725`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
