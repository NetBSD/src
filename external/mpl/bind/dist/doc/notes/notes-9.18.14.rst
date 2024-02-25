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

Notes for BIND 9.18.14
----------------------

Removed Features
~~~~~~~~~~~~~~~~

- Zone type ``delegation-only``, and the ``delegation-only`` and
  ``root-delegation-only`` statements, have been deprecated.
  A warning is now logged when they are used.

  These statements were created to address the SiteFinder controversy,
  in which certain top-level domains redirected misspelled queries to
  other sites instead of returning NXDOMAIN responses. Since top-level
  domains are now DNSSEC-signed, and DNSSEC validation is active by
  default, the statements are no longer needed. :gl:`#3953`

Bug Fixes
~~~~~~~~~

- Several bugs which could cause :iscman:`named` to crash during catalog
  zone processing have been fixed. :gl:`#3955` :gl:`#3968` :gl:`#3997`

- Previously, downloading large zones over TLS (XoT) from a primary
  could hang the transfer on the secondary, especially when the
  connection was unstable. This has been fixed. :gl:`#3867`

- Performance of DNSSEC validation in zones with many DNSKEY records has
  been improved. :gl:`#3981`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
