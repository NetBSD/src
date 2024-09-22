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

Notes for BIND 9.18.27
----------------------

New Features
~~~~~~~~~~~~

- A new option :any:`signatures-jitter` has been added to :any:`dnssec-policy`
  to allow signature expirations to be spread out over a period of time.
  :gl:`#4554`

Feature Changes
~~~~~~~~~~~~~~~

- DNSSEC signatures that are not valid because the current time falls outside
  the signature inception and expiration dates are skipped instead of causing
  an immediate validation failure. :gl:`#4586`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
