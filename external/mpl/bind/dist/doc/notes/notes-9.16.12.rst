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

Notes for BIND 9.16.12
----------------------

Security Fixes
~~~~~~~~~~~~~~

- When ``tkey-gssapi-keytab`` or ``tkey-gssapi-credential`` was
  configured, a specially crafted GSS-TSIG query could cause a buffer
  overflow in the ISC implementation of SPNEGO (a protocol enabling
  negotiation of the security mechanism to use for GSSAPI
  authentication). This flaw could be exploited to crash ``named``.
  Theoretically, it also enabled remote code execution, but achieving
  the latter is very difficult in real-world conditions.
  (CVE-2020-8625)

  This vulnerability was responsibly reported to us as ZDI-CAN-12302 by
  Trend Micro Zero Day Initiative. :gl:`#2354`

New Features
~~~~~~~~~~~~

- When a secondary server receives a large incremental zone transfer
  (IXFR), it can have a negative impact on query performance while the
  incremental changes are applied to the zone. To address this,
  ``named`` can now limit the size of IXFR responses it sends in
  response to zone transfer requests. If an IXFR response would be
  larger than an AXFR of the entire zone, it will send an AXFR response
  instead.

  This behavior is controlled by the ``max-ixfr-ratio`` option - a
  percentage value representing the ratio of IXFR size to the size of a
  full zone transfer. The default is ``100%``. :gl:`#1515`

- A new option, ``stale-answer-client-timeout``, has been added to
  improve ``named``'s behavior with respect to serving stale data. The
  option defines the amount of time ``named`` waits before attempting to
  answer the query with a stale RRset from cache. If a stale answer is
  found, ``named`` continues the ongoing fetches, attempting to refresh
  the RRset in cache until the ``resolver-query-timeout`` interval is
  reached.

  The default value is ``1800`` (in milliseconds) and the maximum value
  is limited to ``resolver-query-timeout`` minus one second. A value of
  ``0`` causes any available cached RRset to immediately be returned
  while still triggering a refresh of the data in cache.

  This new behavior can be disabled by setting
  ``stale-answer-client-timeout`` to ``off`` or ``disabled``. The new
  option has no effect if ``stale-answer-enable`` is disabled.
  :gl:`#2247`

Feature Changes
~~~~~~~~~~~~~~~

- As part of an ongoing effort to use :rfc:`8499` terminology,
  ``primaries`` can now be used as a synonym for ``masters`` in
  ``named.conf``. Similarly, ``notify primary-only`` can now be used as
  a synonym for ``notify master-only``. The output of ``rndc
  zonestatus`` now uses ``primary`` and ``secondary`` terminology.
  :gl:`#1948`

- The default value of ``max-stale-ttl`` has been changed from 12 hours
  to 1 day and the default value of ``stale-answer-ttl`` has been
  changed from 1 second to 30 seconds, following :rfc:`8767`
  recommendations. :gl:`#2248`

- The SONAMEs for BIND 9 libraries now include the current BIND 9
  version number, in an effort to tightly couple internal libraries with
  a specific release. This change makes the BIND 9 release process both
  simpler and more consistent while also unequivocally preventing BIND 9
  binaries from silently loading wrong versions of shared libraries (or
  multiple versions of the same shared library) at startup. :gl:`#2387`

- When ``check-names`` is in effect, A records below an ``_spf``,
  ``_spf_rate``, or ``_spf_verify`` label (which are employed by the
  ``exists`` SPF mechanism defined in :rfc:`7208` section 5.7/appendix
  D.1) are no longer reported as warnings/errors. :gl:`#2377`

Bug Fixes
~~~~~~~~~

- ``named`` failed to start when its configuration included a zone with
  a non-builtin ``allow-update`` ACL attached. :gl:`#2413`

- Previously, ``dnssec-keyfromlabel`` crashed when operating on an ECDSA
  key. This has been fixed. :gl:`#2178`

- KASP incorrectly set signature validity to the value of the DNSKEY
  signature validity. This has been fixed. :gl:`#2383`

- When migrating to KASP, BIND 9 considered keys with the ``Inactive``
  and/or ``Delete`` timing metadata to be possible active keys. This has
  been fixed. :gl:`#2406`

- Fix the "three is a crowd" key rollover bug in KASP. When keys rolled
  faster than the time required to finish the rollover procedure, the
  successor relation equation failed because it assumed only two keys
  were taking part in a rollover. This could lead to premature removal
  of predecessor keys. BIND 9 now implements a recursive successor
  relation, as described in the paper "Flexible and Robust Key Rollover"
  (Equation (2)). :gl:`#2375`

- Performance of the DNSSEC verification code (used by
  ``dnssec-signzone``, ``dnssec-verify``, and mirror zones) has been
  improved. :gl:`#2073`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
