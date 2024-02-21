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

Notes for BIND 9.18.1
---------------------

Security Fixes
~~~~~~~~~~~~~~

- The rules for acceptance of records into the cache have been tightened
  to prevent the possibility of poisoning if forwarders send records
  outside the configured bailiwick. :cve:`2021-25220`

  ISC would like to thank Xiang Li, Baojun Liu, and Chaoyi Lu from
  Network and Information Security Lab, Tsinghua University, and
  Changgen Zou from Qi An Xin Group Corp. for bringing this
  vulnerability to our attention. :gl:`#2950`

- TCP connections with :any:`keep-response-order` enabled could leave the
  TCP sockets in the ``CLOSE_WAIT`` state when the client did not
  properly shut down the connection. :cve:`2022-0396` :gl:`#3112`

- Lookups involving a DNAME could trigger an assertion failure when
  :any:`synth-from-dnssec` was enabled (which is the default).
  :cve:`2022-0635`

  ISC would like to thank Vincent Levigneron from AFNIC for bringing
  this vulnerability to our attention. :gl:`#3158`

- When chasing DS records, a timed-out or artificially delayed fetch
  could cause ``named`` to crash while resuming a DS lookup.
  :cve:`2022-0667` :gl:`#3129`

Feature Changes
~~~~~~~~~~~~~~~

- The DLZ API has been updated: EDNS Client-Subnet (ECS) options sent
  by a client are now included in the client information sent to DLZ
  modules when processing queries. :gl:`#3082`

- DEBUG(1)-level messages were added when starting and ending the BIND 9
  task-exclusive mode that stops normal DNS operation (e.g. for
  reconfiguration, interface scans, and other events that require
  exclusive access to a shared resource). :gl:`#3137`

- The limit on the number of simultaneously processed pipelined DNS
  queries received over TCP has been removed. Previously, it was capped
  at 23 queries processed at the same time. :gl:`#3141`

Bug Fixes
~~~~~~~~~

- A failed view configuration during a ``named`` reconfiguration
  procedure could cause inconsistencies in BIND internal structures,
  causing a crash or other unexpected errors. This has been fixed.
  :gl:`#3060`

- Previously, ``named`` logged a "quota reached" message when it hit its
  hard quota on the number of connections. That message was accidentally
  removed but has now been restored. :gl:`#3125`

- The :any:`max-transfer-time-out` and :any:`max-transfer-idle-out` options
  were not implemented when the BIND 9 networking stack was refactored
  in 9.16. The missing functionality has been re-implemented and
  outgoing zone transfers now time out properly when not progressing.
  :gl:`#1897`

- TCP connections could hang indefinitely if the other party did not
  read sent data, causing the TCP write buffers to fill. This has been
  fixed by adding a "write" timer. Connections that are hung while
  writing now time out after the :any:`tcp-idle-timeout` period has
  elapsed. :gl:`#3132`

- Client TCP connections are now closed immediately when data received
  cannot be parsed as a valid DNS request. :gl:`#3149`

- The statistics counter representing the current number of clients
  awaiting recursive resolution results (``RecursClients``) could be
  miscalculated in certain resolution scenarios, potentially causing the
  value of the counter to drop below zero. This has been fixed.
  :gl:`#3147`

- An error in the processing of the :any:`blackhole` ACL could cause some
  DNS requests sent by :iscman:`named` to fail - for example, zone
  transfer requests and SOA refresh queries - if the destination address
  or prefix was specifically excluded from the ACL using ``!``, or if
  the ACL was set to ``none``. This has now been fixed. :any:`blackhole`
  worked correctly when it was left unset, or if only positive-match
  elements were included. :gl:`#3157`

- Build errors were introduced in some DLZ modules due to an incomplete
  change in the previous release. This has been fixed. :gl:`#3111`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
