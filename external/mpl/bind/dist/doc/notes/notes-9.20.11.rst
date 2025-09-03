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

Notes for BIND 9.20.11
----------------------

Security Fixes
~~~~~~~~~~~~~~

- Fix a possible assertion failure when
  :any:`stale-answer-client-timeout` is set to ``0``.

  In specific circumstances the :iscman:`named` resolver process could
  exit with an assertion failure when stale answers were enabled and the
  :any:`stale-answer-client-timeout` configuration option was set to
  ``0``. This has been fixed. :cve:`2025-40777` :gl:`#5372`

New Features
~~~~~~~~~~~~

- Add support for the CO flag to :iscman:`dig`.

  Add support for Compact Denial of Existence to :iscman:`dig`.  This
  includes showing the CO (Compact Answers OK) flag when displaying
  messages and adding an option to set the CO flag when making queries
  (:option:`dig +coflag`). :gl:`#5319`

Bug Fixes
~~~~~~~~~

- Correct the default :any:`interface-interval` from 60s to 60m.

  When the :any:`interface-interval` parser was changed from a
  ``uint32`` parser to a duration parser, the default value stayed at
  plain number ``60`` which now means 60 seconds instead of 60 minutes.
  The documentation also incorrectly states that the value is in
  minutes. That has been fixed. :gl:`#5246`

- Fix a :any:`purge-keys` bug when using multiple views of a zone.

  Previously, when a DNSSEC key was purged by one zone view, other zone
  views would return an error about missing key files. This has been
  fixed. :gl:`#5315`

- Use IPv6 queries in :option:`delv +ns`.

  :option:`delv +ns` invokes the same code to perform name resolution as
  :iscman:`named`, but it neglected to set up an IPv6 dispatch object
  first. Consequently, it was behaving more like :option:`named -4`. It
  now sets up dispatch objects for both address families, and performs
  resolver queries to both IPv4 and IPv6 addresses, except when one of
  the address families has been suppressed by using :option:`delv -4` or
  :option:`delv -6`. :gl:`#5352`
