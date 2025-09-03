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

BIND 9.20.11
------------

Security Fixes
~~~~~~~~~~~~~~

- [CVE-2025-40777] Fix a possible assertion failure when using the
  'stale-answer-client-timeout 0' option. ``055a592fd97``

  In specific circumstances the :iscman:`named` resolver process could
  terminate unexpectedly when stale answers were enabled and the
  ``stale-answer-client-timeout 0`` configuration option was used. This
  has been fixed. :gl:`#5372`

New Features
~~~~~~~~~~~~

- Add support for the CO flag to dig. ``47108af9f2e``

  Add support to display the CO (Compact Answers OK flag)
  when displaying messages.

  Add support to set the CO flag when making queries in dig (+coflag).
  :gl:`#5319` :gl:`!10578`

Bug Fixes
~~~~~~~~~

- Fix the default interface-interval from 60s to 60m. ``e8ffe3a15ca``

  When the interface-interval parser was changed from uint32 parser to
  duration parser, the default value stayed at plain number `60` which
  now means 60 seconds instead of 60 minutes.  The documentation also
  incorrectly states that the value is in minutes.  That has been fixed.
  :gl:`#5246` :gl:`!10679`

- Fix purge-keys bug when using views. ``35efa742b03``

  Previously, when a DNSSEC key was purged by one zone view, other zone
  views would return an error about missing key files. This has been
  fixed. :gl:`#5315` :gl:`!10598`

- Use IPv6 queries in delv +ns. ``4916fe0c6bd``

  `delv +ns` invokes the same code to perform name resolution as
  `named`, but it neglected to set up an IPv6 dispatch object first.
  Consequently, it was behaving more like `named -4`. It now sets up
  dispatch objects for both address families, and performs resolver
  queries to both v4 and v6 addresses, except when one of the address
  families has been suppressed by using `delv -4` or `delv -6`.
  :gl:`#5352` :gl:`!10573`


