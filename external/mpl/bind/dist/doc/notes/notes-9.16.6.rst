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

Notes for BIND 9.16.6
---------------------

Security Fixes
~~~~~~~~~~~~~~

- It was possible to trigger an assertion failure by sending a specially
  crafted large TCP DNS message. This was disclosed in CVE-2020-8620.

  ISC would like to thank Emanuel Almeida of Cisco Systems, Inc. for
  bringing this vulnerability to our attention. :gl:`#1996`

- ``named`` could crash after failing an assertion check in certain
  query resolution scenarios where QNAME minimization and forwarding
  were both enabled. To prevent such crashes, QNAME minimization is now
  always disabled for a given query resolution process, if forwarders
  are used at any point. This was disclosed in CVE-2020-8621.

  ISC would like to thank Joseph Gullo for bringing this vulnerability
  to our attention. :gl:`#1997`

- It was possible to trigger an assertion failure when verifying the
  response to a TSIG-signed request. This was disclosed in
  CVE-2020-8622.

  ISC would like to thank Dave Feldman, Jeff Warren, and Joel Cunningham
  of Oracle for bringing this vulnerability to our attention.
  :gl:`#2028`

- When BIND 9 was compiled with native PKCS#11 support, it was possible
  to trigger an assertion failure in code determining the number of bits
  in the PKCS#11 RSA public key with a specially crafted packet. This
  was disclosed in CVE-2020-8623.

  ISC would like to thank Lyu Chiy for bringing this vulnerability to
  our attention. :gl:`#2037`

- ``update-policy`` rules of type ``subdomain`` were incorrectly treated
  as ``zonesub`` rules, which allowed keys used in ``subdomain`` rules
  to update names outside of the specified subdomains. The problem was
  fixed by making sure ``subdomain`` rules are again processed as
  described in the ARM. This was disclosed in CVE-2020-8624.

  ISC would like to thank Joop Boonen of credativ GmbH for bringing this
  vulnerability to our attention. :gl:`#2055`

New Features
~~~~~~~~~~~~

- A new configuration option ``stale-cache-enable`` has been introduced
  to enable or disable keeping stale answers in cache. :gl:`#1712`

Feature Changes
~~~~~~~~~~~~~~~

- BIND's cache database implementation has been updated to use a faster
  hash function with better distribution. In addition, the effective
  ``max-cache-size`` (configured explicitly, defaulting to a value based
  on system memory or set to ``unlimited``) now pre-allocates fixed-size
  hash tables. This prevents interruption to query resolution when the
  hash table sizes need to be increased. :gl:`#1775`

- Resource records received with 0 TTL are no longer kept in the cache
  to be used for stale answers. :gl:`#1829`

Bug Fixes
~~~~~~~~~

- Wildcard RPZ passthru rules could incorrectly be overridden by other
  rules that were loaded from RPZ zones which appeared later in the
  ``response-policy`` statement. This has been fixed. :gl:`#1619`

- The IPv6 Duplicate Address Detection (DAD) mechanism could
  inadvertently prevent ``named`` from binding to new IPv6 interfaces,
  by causing multiple route socket messages to be sent for each IPv6
  address. ``named`` monitors for new interfaces to ``bind()`` to when
  it is configured to listen on ``any`` or on a specific range of
  addresses. New IPv6 interfaces can be in a "tentative" state before
  they are fully available for use. When DAD is in use, two messages are
  emitted by the route socket: one when the interface first appears and
  then a second one when it is fully "up." An attempt by ``named`` to
  ``bind()`` to the new interface prematurely would fail, causing it
  thereafter to ignore that address/interface. The problem was worked
  around by setting the ``IP_FREEBIND`` option on the socket and trying
  to ``bind()`` to each IPv6 address again if the first ``bind()`` call
  for that address failed with ``EADDRNOTAVAIL``. :gl:`#2038`

- Addressed an error in recursive clients stats reporting which could
  cause underflow, and even negative statistics. There were occasions
  when an incoming query could trigger a prefetch for some eligible
  RRset, and if the prefetch code were executed before recursion, no
  increment in recursive clients stats would take place. Conversely,
  when processing the answers, if the recursion code were executed
  before the prefetch, the same counter would be decremented without a
  matching increment. :gl:`#1719`

- The introduction of KASP support inadvertently caused the second field
  of ``sig-validity-interval`` to always be calculated in hours, even in
  cases when it should have been calculated in days. This has been
  fixed. (Thanks to Tony Finch.) :gl:`!3735`

- LMDB locking code was revised to make ``rndc reconfig`` work properly
  on FreeBSD and with LMDB >= 0.9.26. :gl:`#1976`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
