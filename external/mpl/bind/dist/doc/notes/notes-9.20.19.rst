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

Notes for BIND 9.20.19
----------------------

Feature Changes
~~~~~~~~~~~~~~~

- Update requirements for system test suite.

  Python 3.10 or newer is now required for running the system test suite. The
  required Python packages and their version requirements are now tracked in the
  file `bin/tests/system/requirements.txt`. :gl:`#5690` :gl:`#5614`


Bug Fixes
~~~~~~~~~

- Fix inbound IXFR performance regression.

  Very large inbound IXFR transfers were much slower compared to BIND
  9.18. The performance was improved by adding specialized logic to
  handle IXFR transfers. :gl:`#5442`

- Make catalog zone names and member zones' entry names
  case-insensitive. :gl:`#5693`

- Fix implementation of BRID and HHIT record types. :gl:`#5710`

- Fix implementation of DSYNC record type. :gl:`#5711`

- Fix response policy and catalog zones to work with `$INCLUDE` directive.

  Reloading a RPZ or a catalog zone could have failed when `$INCLUDE` was in use. :gl:`#5714`
