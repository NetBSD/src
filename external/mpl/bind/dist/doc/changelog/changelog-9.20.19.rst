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

BIND 9.20.19
------------

Feature Changes
~~~~~~~~~~~~~~~

- Update requirements for system test suite. ``a2a9b1b878``

  Python 3.10 or newer is now required for running the system test
  suite. The required python packages and their version requirements are
  now tracked in `bin/tests/system/requirements.txt`.

  Support for pytest 9.0.0 has been added its minimum supported version
  has been raised to 7.0.0. The minimum supported dnspython version has
  been raised to 2.3.0. :gl:`#5690`  :gl:`#5614` :gl:`!11469`

- Use enum rather than numbers for isc_base64_tobuffer and
  isc_hex_tobuffer. ``47b8ca5ac7``

  Use isc_one_or_more and isc_zero_or_more rather than (-2) and (-1)
  when calling isc_base64_tobuffer. Similarly for isc_hex_tobuffer. This
  should help reduce the probability that the wrong number is used and
  it makes the intent clearer. :gl:`#5713` :gl:`!11498`

Bug Fixes
~~~~~~~~~

- Fix inbound IXFR performance regression. ``318a7535d2``

  Very large inbound IXFR transfers were much slower compared to BIND
  9.18. The performance was improved by adding specialized logic to
  handle IXFR transfers. :gl:`#5442` :gl:`!11355`

- Make catalog zone names and member zones' entry names
  case-insensitive. ``cd23f0250a``

  Previously, the catalog zone names and their member zones' entry names
  were unintentionally case-sensitive. This has been fixed. :gl:`#5693`
  :gl:`!11450`

- Use const pointer with strchr of const pointer. ``736b84ad46``

  :gl:`#5694` :gl:`!11463`

- Fix brid and hhit implementation. ``f73ef3b24f``

  Fix bugs in BRID and HHIT implementation and enable the unit tests.
  :gl:`#5710` :gl:`!11492`

- DSYNC record incorrectly used two octets for the Scheme Field.
  ``bd9f73c705``

  When creating the `DSYNC` record from a structure, `uint16_tobuffer`
  was used instead of `uint8_tobuffer` when adding the scheme, causing a
  `DSYNC` record that was one octet too long. This has been fixed.
  :gl:`#5711` :gl:`!11483`

- Fix a possible issue with reponse policy zones and catalog zones.
  ``3d0823ee68``

  If a response policy zone (RPZ) or a catalog zone contained an
  `$INCLUDE` directive, then manually reloading that zone could fail to
  process the changes in the response policy or in the catalog,
  respectively. This has been fixed. :gl:`#5714` :gl:`!11496`


