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

Notes for BIND 9.20.1
---------------------

New Features
~~~~~~~~~~~~

- Implement ``rndc retransfer -force``.

  A new optional argument ``-force`` has been added to the command
  :option:`rndc retransfer`. When it is specified, :iscman:`named` aborts the
  ongoing zone transfer (if there is one) and starts a new transfer.
  :gl:`#2299` :gl:`!9219`

- :iscman:`dig` now reports a missing QUESTION section for messages with opcode QUERY.

  Query responses should contain the QUESTION section, with some
  exceptions. :iscman:`dig` was not reporting this. :gl:`#4808` :gl:`!9269`

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
  the issue. :gl:`#4741` :gl:`!9282`

- Allow shorter :any:`resolver-query-timeout` configuration.

  The minimum allowed value of :any:`resolver-query-timeout` was lowered
  from its previous value of 10 000 milliseconds (which is still the default)
  to 301 milliseconds. Note however that values of 1 to 300 inclusive are
  interpreted as seconds before applying the limit. A value of zero is
  interpreted as the default. :gl:`#4320` :gl:`!9220`

- Raise the log level of priming failures.

  When a priming query is complete, it was previously logged at level
  ``DEBUG(1)``, regardless of success or failure. It is now
  logged to ``NOTICE`` in the case of failure. :gl:`#3516`
  :gl:`!9250`

Bug Fixes
~~~~~~~~~

- Fix a crash caused by valid TSIG signatures with invalid time.

  An assertion failure was triggered when the TSIG had a valid
  cryptographic signature but the time was invalid. This could happen
  when the times between the primary and secondary servers were not
  synchronised. The crash has now been fixed. :gl:`#4811` :gl:`!9245`

- Return SERVFAIL for a too long CNAME chain.

  When following long CNAME chains, :iscman:`named` was returning NOERROR
  (along with a partial answer) instead of SERVFAIL, if the chain exceeded the
  maximum length. This has been fixed. :gl:`#4449` :gl:`!9203`

- Reconfigure catz member zones during :iscman:`named` reconfiguration.

  During a reconfiguration, :iscman:`named` wasn't reconfiguring catalog
  zones' member zones. This has been fixed. :gl:`#4733`

- Update key lifetime and metadata after :any:`dnssec-policy` reconfiguration.

  Adjust key state and timing metadata if :any:`dnssec-policy` key
  lifetime configuration is updated, so that it also affects existing
  keys. :gl:`#4677` :gl:`!9191`

- Fix a crash during zone modification.

  Fix an assertion failure that could happen when an authoritative zone was
  modified while the server was generating an answer from that zone.
  :gl:`#4691` :gl:`!9126`

- Fix assertion failure when executing :option:`named-checkconf -v`
  to print its version. :gl:`#4827` :gl:`!9246`

- Fix generation of 6to4-self name expansion from IPv4 address.

  The period between the most significant nibble of the encoded IPv4
  address and the 2.0.0.2.IP6.ARPA suffix was missing, resulting in the
  wrong name being checked. This has been fixed. :gl:`#4766` :gl:`!9217`

- :option:`dig +yaml` was producing unexpected and/or invalid YAML.
  output. :gl:`#4796` :gl:`!9213`

- SVBC ALPN text parsing failed to reject zero-length ALPN. :gl:`#4775` :gl:`!9209`

- Fix false QNAME minimisation error being reported.

  Remove the false positive ``success resolving`` log message when QNAME
  minimisation is in effect and the final result is an NXDOMAIN. :gl:`#4784` :gl:`!9215`

- Fix ``--enable-tracing`` build on systems without dtrace.

  A missing ``util/dtrace.sh`` file prevented builds on systems without
  the ``dtrace`` utility. This has been corrected. :gl:`#4835` :gl:`!9272`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
