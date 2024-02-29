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

Notes for BIND 9.18.21
----------------------

Removed Features
~~~~~~~~~~~~~~~~

- Support for using AES as the DNS COOKIE algorithm (``cookie-algorithm
  aes;``) has been deprecated and will be removed in a future release.
  Please use the current default, SipHash-2-4, instead. :gl:`#4421`

- The :any:`resolver-nonbackoff-tries` and :any:`resolver-retry-interval`
  statements have been deprecated. Using them now causes a warning to be
  logged. :gl:`#4405`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
