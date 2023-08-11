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

Notes for BIND 9.16.22
----------------------

Security Fixes
~~~~~~~~~~~~~~

- The ``lame-ttl`` option controls how long ``named`` caches certain
  types of broken responses from authoritative servers (see the
  `security advisory <https://kb.isc.org/docs/cve-2021-25219>`_ for
  details). This caching mechanism could be abused by an attacker to
  significantly degrade resolver performance. The vulnerability has been
  mitigated by changing the default value of ``lame-ttl`` to ``0`` and
  overriding any explicitly set value with ``0``, effectively disabling
  this mechanism altogether. ISC's testing has determined that doing
  that has a negligible impact on resolver performance while also
  preventing abuse. Administrators may observe more traffic towards
  servers issuing certain types of broken responses than in previous
  BIND 9 releases, depending on client query patterns. (CVE-2021-25219)

  ISC would like to thank Kishore Kumar Kothapalli of Infoblox for
  bringing this vulnerability to our attention. :gl:`#2899`

Feature Changes
~~~~~~~~~~~~~~~

- The use of native PKCS#11 for Public-Key Cryptography in BIND 9 has
  been deprecated in favor of the engine_pkcs11 OpenSSL engine from the
  `OpenSC`_ project. The ``--with-native-pkcs11`` configuration option
  will be removed in the next major BIND 9 release. The option to use
  the engine_pkcs11 OpenSSL engine is already available in BIND 9;
  please see the :ref:`ARM section on PKCS#11 <pkcs11>` for details.
  :gl:`#2691`

- Old-style Dynamically Loadable Zones (DLZ) drivers that had to be
  enabled in ``named`` at build time have been marked as deprecated in
  favor of new-style DLZ modules. Old-style DLZ drivers will be removed
  in the next major BIND 9 release. :gl:`#2814`

- The ``map`` zone file format has been marked as deprecated and will be
  removed in the next major BIND 9 release. :gl:`#2882`

- ``named`` and ``named-checkconf`` now exit with an error when a single
  port configured for ``query-source``, ``transfer-source``,
  ``notify-source``, ``parental-source``, and/or their respective IPv6
  counterparts clashes with a global listening port. This configuration
  has not been supported since BIND 9.16.0, but no error was reported
  until now (even though sending UDP messages such as NOTIFY failed).
  :gl:`#2888`

- ``named`` and ``named-checkconf`` now issue a warning when there is a
  single port configured for ``query-source``, ``transfer-source``,
  ``notify-source``, ``parental-source``, and/or for their respective
  IPv6 counterparts. :gl:`#2888`

.. _OpenSC: https://github.com/OpenSC/libp11

Bug Fixes
~~~~~~~~~

- A recent change introduced in BIND 9.16.21 inadvertently broke
  backward compatibility for the ``check-names master ...`` and
  ``check-names slave ...`` options, causing them to be silently
  ignored. This has been fixed and these options now work properly
  again. :gl:`#2911`

- When new IP addresses were set up by the operating system during
  ``named`` startup, it could fail to listen for TCP connections on the
  newly added interfaces. :gl:`#2852`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
