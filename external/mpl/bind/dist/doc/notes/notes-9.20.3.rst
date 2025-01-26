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

Notes for BIND 9.20.3
---------------------

New Features
~~~~~~~~~~~~

- Log query response status to the query log.

  Log a query response summary using the new ``responses`` category.
  Logging can be controlled via the :any:`responselog` option and via
  :option:`rndc responselog`. :gl:`#459`

- Added WALLET type.

  Add the new record type WALLET (262).  This provides a mapping from a
  domain name to a cryptographic currency wallet.  Multiple mappings can
  exist if multiple records exist. :gl:`#4947`

Feature Changes
~~~~~~~~~~~~~~~

- Set logging category for ``notify``/``xfer-in``-related messages.

  Some ``notify`` and ``xfer-in``-related log messages were logged at
  the "general" category level instead of their own category. This has
  been fixed.  :gl:`#2730`

- Allow IXFR-to-AXFR fallback on ``DNS_R_TOOMANYRECORDS``.

  This change allows fallback from an IXFR failure to AXFR when the
  reason is ``DNS_R_TOOMANYRECORDS``. :gl:`#4928`

Bug Fixes
~~~~~~~~~

- Fix a statistics channel counter bug when "forward only" zones are
  used.

  When resolving a zone with a "forward only" policy, and finding out
  that all the forwarders were marked as "bad", the "ServerQuota"
  counter of the statistics channel was incorrectly increased. This has
  been fixed. :gl:`#1793`

- Fix a bug in the static-stub implementation.

  Static-stub addresses and addresses from other sources were being
  mixed together, resulting in static-stub queries going to addresses
  not specified in the configuration, or alternatively, static-stub
  addresses being used instead of the correct server addresses.
  :gl:`#4850`

- Don't allow :any:`statistics-channels` if libxml2 and libjson-c are
  not configured.

  When BIND 9 is not configured with the libxml2 and libjson-c
  libraries, the use of the :any:`statistics-channels` option is a fatal
  error.  :gl:`#4895`

- Separate DNSSEC validation from long-running tasks.

  Split CPU-intensive and long-running tasks into separate threadpools
  in a way that the long-running tasks - like RPZ, catalog zone
  processing, or zone file operations - don't block CPU-intensive
  operations like DNSSEC validations. :gl:`#4898`

- Fix an assertion failure when processing access control lists.

  The :iscman:`named` process could terminate unexpectedly when
  processing ACLs.  This has been fixed. :gl:`#4908`

- Fix a bug in Offline KSK using a ZSK with an unlimited lifetime.

  If the ZSK had an unlimited lifetime, the timing metadata ``Inactive``
  and ``Delete`` could not be found and were treated as an error,
  preventing the zone from being signed. This has been fixed.
  :gl:`#4914`

- Limit the outgoing UDP send queue size.

  If the operating system UDP queue got full and the outgoing UDP
  sending started to be delayed, BIND 9 could exhibit memory spikes as
  it tried to enqueue all the outgoing UDP messages. It now tries to
  deliver the outgoing UDP messages synchronously; if that fails, it
  drops the outgoing DNS message that would get queued up and then
  timeout on the client side. :gl:`#4930`

- Do not set ``SO_INCOMING_CPU``.

  Remove the ``SO_INCOMING_CPU`` setting as kernel scheduling performs
  better without constraints. :gl:`#4936`

- Fix the :option:`rndc dumpdb` command's error reporting.

  The :option:`rndc dumpdb` command was not reporting errors that
  occurred when :iscman:`named` started up the database dump process.
  This has been fixed. :gl:`#4944`

- Fix long-running incoming transfers.

  Incoming transfers that took longer than 30 seconds would stop reading
  from the TCP stream and the incoming transfer would be indefinitely
  stuck, causing BIND 9 to hang during shutdown.

  This has been fixed, and the :any:`max-transfer-time-in` and
  :any:`max-transfer-idle-in` timeouts are now honored. :gl:`#4949`

- Fix an assertion failure when receiving DNS responses over TCP.

  When matching the received Query ID in the TCP connection, an invalid
  Query ID could cause an assertion failure. This has been fixed.
  :gl:`#4952`


Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
