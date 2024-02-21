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

Notes for BIND 9.18.3
---------------------

Security Fixes
~~~~~~~~~~~~~~

- Previously, TLS socket objects could be destroyed prematurely, which
  triggered assertion failures in :iscman:`named` instances serving
  DNS-over-HTTPS (DoH) clients. This has been fixed.

  ISC would like to thank Thomas Amgarten from arcade solutions ag for
  bringing this vulnerability to our attention. :cve:`2022-1183`
  :gl:`#3216`

Known Issues
~~~~~~~~~~~~

- According to :rfc:`8310`, Section 8.1, the ``Subject`` field MUST NOT
  be inspected when verifying a remote certificate while establishing a
  DNS-over-TLS connection. Only ``subjectAltName`` must be checked
  instead. Unfortunately, some quite old versions of cryptographic
  libraries might lack the ability to ignore the ``Subject`` field. This
  should have minimal production-use consequences, as most of the
  production-ready certificates issued by certificate authorities will
  have ``subjectAltName`` set. In such cases, the ``Subject`` field is
  ignored. Only old platforms are affected by this, e.g. those supplied
  with OpenSSL versions older than 1.1.1. :gl:`#3163`

- See :ref:`above <relnotes_known_issues>` for a list of all known
  issues affecting this BIND 9 branch.

New Features
~~~~~~~~~~~~

- Catalog Zones schema version 2, as described in the
  "DNS Catalog Zones" IETF draft version 5 document, is now supported by
  :iscman:`named`. All of the previously supported BIND-specific catalog
  zone custom properties (:any:`primaries`, :any:`allow-query`, and
  :any:`allow-transfer`), as well as the new Change of Ownership (``coo``)
  property, are now implemented. Schema version 1 is still supported,
  with some additional validation rules applied from schema version 2:
  for example, the :any:`version` property is mandatory, and a member zone
  PTR RRset must not contain more than one record. In the event of a
  validation error, a corresponding error message is logged to help with
  diagnosing the problem. :gl:`#3221` :gl:`#3222` :gl:`#3223`
  :gl:`#3224` :gl:`#3225`

- Support DNS Extended Errors (:rfc:`8914`) ``Stale Answer`` and
  ``Stale NXDOMAIN Answer`` when stale answers are returned from cache.
  :gl:`#2267`

- Add support for remote TLS certificate verification, both to
  :iscman:`named` and :iscman:`dig`, making it possible to implement
  Strict and Mutual TLS authentication, as described in :rfc:`9103`,
  Section 9.3. :gl:`#3163`

Bug Fixes
~~~~~~~~~

- Previously, CDS and CDNSKEY DELETE records were removed from the zone
  when configured with the ``auto-dnssec maintain;`` option. This has
  been fixed. :gl:`#2931`
