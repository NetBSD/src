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

Notes for BIND 9.18.26
----------------------

New Features
~~~~~~~~~~~~

- The statistics channel now includes counters that indicate the number
  of currently connected TCP IPv4/IPv6 clients. :gl:`#4425`

- Added RESOLVER.ARPA to the built in empty zones. :gl:`#4580`

Bug Fixes
~~~~~~~~~

- Changes to ``listen-on`` statements were ignored on reconfiguration
  unless the port or interface address was changed, making it
  impossible to change a related listener transport type. That issue
  has been fixed.

  ISC would like to thank Thomas Amgarten for bringing this issue to
  our attention. :gl:`#4518` :gl:`#4528`

- A bug in the keymgr code unintentionally slowed down some DNSSEC key
  rollovers. This has been fixed. :gl:`#4552`

- Some ISO 8601 durations were accepted erroneously, leading to shorter
  durations than expected. This has been fixed. :gl:`#4624`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
