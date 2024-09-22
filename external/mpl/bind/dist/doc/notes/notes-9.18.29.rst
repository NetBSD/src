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

Notes for BIND 9.18.29
----------------------

Feature Changes
~~~~~~~~~~~~~~~

- Tighten :any:`max-recursion-queries` and add :any:`max-query-restarts`
  configuration statement.

  There were cases when the :any:`max-recursion-queries`
  quota was ineffective. It was possible to craft zones that would cause
  a resolver to waste resources by sending excessive queries while
  attempting to resolve a name. This has been addressed by correcting
  errors in the implementation of :any:`max-recursion-queries` and by
  reducing the default value from 100 to 32.

  In addition, a new :any:`max-query-restarts` configuration statement has been
  added, which limits the number of times a recursive server will follow CNAME
  or DNAME records before terminating resolution. This was previously a
  hard-coded limit of 16 but is now configurable with a default value of 11.

  ISC would like to thank Huayi Duan, Marco Bearzi, Jodok Vieli, and Cagin
  Tanir from NetSec group, ETH Zurich for discovering and notifying us about
  the issue. :gl:`#4741` :gl:`!9283`

- Raise the log level of priming failures.

  When a priming query is complete, it was previously logged at level
  ``DEBUG(1)``, regardless of success or failure. It is now
  logged to ``NOTICE`` in the case of failure. :gl:`#3516`
  :gl:`!9251`

- Add a compatibility shim for older libuv versions (< 1.19.0)

  The function uv_stream_get_write_queue_size() is supported only in relatively
  new versions of libuv (1.19.0 or higher). Provide a compatibility
  shim for this function so BIND 9 can be built in environments with
  older libuv versions.

Bug Fixes
~~~~~~~~~

- Return SERVFAIL for a too long CNAME chain.

  When following long CNAME chains, :iscman:`named` was returning NOERROR
  (along with a partial answer) instead of SERVFAIL, if the chain exceeded the
  maximum length. This has been fixed. :gl:`#4449` :gl:`!9204`

- Reconfigure catz member zones during :iscman:`named` reconfiguration.

  During a reconfiguration, :iscman:`named` wasn't reconfiguring catalog
  zones' member zones. This has been fixed. :gl:`#4733`

- Update key lifetime and metadata after :any:`dnssec-policy` reconfiguration.

  Adjust key state and timing metadata if :any:`dnssec-policy` key
  lifetime configuration is updated, so that it also affects existing
  keys. :gl:`#4677` :gl:`!9192`

- Fix generation of 6to4-self name expansion from IPv4 address.

  The period between the most significant nibble of the encoded IPv4
  address and the 2.0.0.2.IP6.ARPA suffix was missing, resulting in the
  wrong name being checked. This has been fixed. :gl:`#4766` :gl:`!9218`

- :option:`dig +yaml` was producing unexpected and/or invalid YAML.
  output. :gl:`#4796` :gl:`!9214`

- SVBC ALPN text parsing failed to reject zero-length ALPN. :gl:`#4775` :gl:`!9210`

- Fix false QNAME minimisation error being reported.

  Remove the false positive ``success resolving`` log message when QNAME
  minimisation is in effect and the final result is an NXDOMAIN.
  :gl:`#4784` :gl:`!9216`

- Fix dig +timeout argument when using +https.

  The +timeout argument was not used on DoH connections. This has been
  fixed. :gl:`#4806` :gl:`!9161`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
