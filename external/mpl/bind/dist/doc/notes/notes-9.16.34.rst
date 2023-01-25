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

Notes for BIND 9.16.34
----------------------

Known Issues
~~~~~~~~~~~~

- Upgrading from BIND 9.16.32 or any older version may require a manual
  configuration change. The following configurations are affected:

  - ``type primary`` zones configured with ``dnssec-policy`` but without
    either ``allow-update`` or ``update-policy``,
  - ``type secondary`` zones configured with ``dnssec-policy``.

  In these cases please add ``inline-signing yes;`` to the individual
  zone configuration(s). Without applying this change, :iscman:`named`
  will fail to start. For more details, see
  https://kb.isc.org/docs/dnssec-policy-requires-dynamic-dns-or-inline-signing

- See :ref:`above <relnotes_known_issues>` for a list of all known
  issues affecting this BIND 9 branch.

New Features
~~~~~~~~~~~~

- Support for parsing and validating the ``dohpath`` service parameter
  in SVCB records was added. :gl:`#3544`

- :iscman:`named` now logs the supported cryptographic algorithms during
  startup and in the output of ``named -V``. :gl:`#3541`

Bug Fixes
~~~~~~~~~

- Changing just the TSIG key names for primaries in catalog zones'
  member zones was not effective. This has been fixed. :gl:`#3557`
