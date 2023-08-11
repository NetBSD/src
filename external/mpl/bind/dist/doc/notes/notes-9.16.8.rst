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

Notes for BIND 9.16.8
---------------------

New Features
~~~~~~~~~~~~

- Add a new ``rndc`` command, ``rndc dnssec -rollover``, which triggers
  a manual rollover for a specific key. :gl:`#1749`

- Add a new ``rndc`` command, ``rndc dumpdb -expired``, which dumps the
  cache database, including expired RRsets that are awaiting cleanup, to
  the ``dump-file`` for diagnostic purposes. :gl:`#1870`

Feature Changes
~~~~~~~~~~~~~~~

- DNS Flag Day 2020: The default EDNS buffer size has been changed from
  4096 to 1232 bytes. According to measurements done by multiple
  parties, this should not cause any operational problems as most of the
  Internet "core" is able to cope with IP message sizes between
  1400-1500 bytes; the 1232 size was picked as a conservative minimal
  number that could be changed by the DNS operator to an estimated path
  MTU minus the estimated header space. In practice, the smallest MTU
  witnessed in the operational DNS community is 1500 octets, the maximum
  Ethernet payload size, so a useful default for maximum DNS/UDP payload
  size on reliable networks would be 1432 bytes. :gl:`#2183`

Bug Fixes
~~~~~~~~~

- ``named`` reported an invalid memory size when running in an
  environment that did not properly report the number of available
  memory pages and/or the size of each memory page. :gl:`#2166`

- With multiple forwarders configured, ``named`` could fail the
  ``REQUIRE(msg->state == (-1))`` assertion in ``lib/dns/message.c``,
  causing it to crash. This has been fixed. :gl:`#2124`

- ``named`` erroneously performed continuous key rollovers for KASP
  policies that used algorithm Ed25519 or Ed448 due to a mismatch
  between created key size and expected key size. :gl:`#2171`

- Updating contents of an RPZ zone which contained names spelled using
  varying letter case could cause some processing rules in that RPZ zone
  to be erroneously ignored. :gl:`#2169`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
