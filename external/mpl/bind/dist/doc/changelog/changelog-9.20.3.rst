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

BIND 9.20.3
-----------

New Features
~~~~~~~~~~~~

- Log query response status to the query log. ``cee11c8610f``

  Log a query response summary using the new category `responses`.
  Logging can be controlled by the option `responselog` and `rndc
  responselog`. :gl:`#459` :gl:`!9526`

- Added WALLET type. ``dad3fafe9eb``

  Add the new record type WALLET (262).  This provides a mapping from a
  domain name to a cryptographic currency wallet.  Multiple mappings can
  exist if multiple records exist. :gl:`#4947` :gl:`!9554`

Feature Changes
~~~~~~~~~~~~~~~

- Set logging category for notify/xfer-in related messages.
  ``1f553c61f76``

  Some 'notify' and 'xfer-in' related log messages were logged at the
  'general' category instead of their own category. This has been fixed.
  :gl:`#2730` :gl:`!9514`

- Restore the number of threadpool threads back to original value.
  ``a0eada53883``

  The issue of long-running operations potentially blocking query
  resolution has been fixed. Revert this temporary workaround and
  restore the number of threadpool threads. :gl:`#4898` :gl:`!9532`

- Allow IXFR-to-AXFR fallback on DNS_R_TOOMANYRECORDS. ``30c4cbd4035``

  This change allows fallback from an IXFR failure to AXFR when the
  reason is `DNS_R_TOOMANYRECORDS`. This is because this error condition
  could be temporary only in an intermediate version of IXFR
  transactions and it's possible that the latest version of the zone
  doesn't have that condition. In such a case, the secondary would never
  be able to update the zone (even if it could) without this fallback.

  This fallback behavior is particularly useful with the recently
  introduced `max-records-per-type` and `max-types-per-name` options:
  the primary may not have these limitations and may temporarily
  introduce "too many" records, breaking IXFR. If the primary side
  subsequently deletes these records, this fallback will help recover
  the zone transfer failure automatically; without it, the secondary
  side would first need to increase the limit, which requires more
  operational overhead and has its own adverse effect. :gl:`#4928`
  :gl:`!9471`

- Remove statslock from dnssec-signzone. ``12eb16186ff``

  Silence Coverity CID 468757 and 468767 (DATA RACE read not locked) by
  converting dnssec-signzone to use atomics for statistics counters
  rather than using a lock. :gl:`#4939` :gl:`!9500`

- Use release memory ordering when incrementing reference counter.
  ``19e3cd0cd2c``

  As the relaxed memory ordering doesn't ensure any memory
  synchronization, it is possible that the increment will succeed even
  in the case when it should not - there is a race between
  atomic_fetch_sub(..., acq_rel) and atomic_fetch_add(..., relaxed).
  Only the result is consistent, but the previous value for both calls
  could be same when both calls are executed at the same time.
  :gl:`!9567`

Bug Fixes
~~~~~~~~~

- Fix a statistics channel counter bug when 'forward only' zones are
  used. ``2287dc0ac0d``

  When resolving a zone with a 'forward only' policy, and finding out
  that all the forwarders are marked as "bad", the 'ServerQuota' counter
  of the statistics channel was incorrectly increased. This has been
  fixed. :gl:`#1793` :gl:`!9502`

- Fix a bug in the static-stub implementation. ``72626cf9405``

  Static-stub addresses and addresses from other sources were being
  mixed together, resulting in static-stub queries going to addresses
  not specified in the configuration, or alternatively, static-stub
  addresses being used instead of the correct server addresses.
  :gl:`#4850` :gl:`!9571`

- Don't allow statistics-channel if libxml2 and libjson-c are
  unsupported. ``02822b70eee``

  When the libxml2 and libjson-c libraries are not supported, the
  statistics channel can't return anything useful, so it is now
  disabled. Use of `statistics-channel` in `named.conf` is a fatal
  error. :gl:`#4895` :gl:`!9486`

- Separate DNSSEC validation from the long-running tasks.
  ``c0022f68025``

  As part of the KeyTrap \[CVE-2023-50387\] mitigation, the DNSSEC CPU-
  intensive operations were offloaded to a separate threadpool that we
  use to run other tasks that could affect the networking latency.

  If that threadpool is running some long-running tasks like RPZ,
  catalog zone processing, or zone file operations, it would delay
  DNSSEC validations to a point where the resolving signed DNS records
  would fail.

  Split the CPU-intensive and long-running tasks into separate
  threadpools in a way that the long-running tasks don't block the CPU-
  intensive operations. :gl:`#4898` :gl:`!9495`

- Fix assertion failure when processing access control lists.
  ``a15d975dbe2``

  The named process could terminate unexpectedly when processing access
  control lists (ACLs). This has been fixed. :gl:`#4908` :gl:`!9466`

- Fix bug in Offline KSK that is using ZSK with unlimited lifetime.
  ``3f115d3cdae``

  If the ZSK has unlimited lifetime, the timing metadata "Inactive" and
  "Delete" cannot be found and is treated as an error, preventing the
  zone to be signed. This has been fixed. :gl:`#4914` :gl:`!9453`

- Fix data race in offloaded dns_message_checksig() ``3b5c4f94d70``

  When verifying a message in an offloaded thread there is a race with
  the worker thread which writes to the same buffer. Clone the message
  buffer before offloading. :gl:`#4929` :gl:`!9490`

- Limit the outgoing UDP send queue size. ``251b90c25e0``

  If the operating system UDP queue gets full and the outgoing UDP
  sending starts to be delayed, BIND 9 could exhibit memory spikes as it
  tries to enqueue all the outgoing UDP messages.  Try a bit harder to
  deliver the outgoing UDP messages synchronously and if that fails,
  drop the outgoing DNS message that would get queued up and then
  timeout on the client side. :gl:`#4930` :gl:`!9511`

- Do not set SO_INCOMING_CPU. ``6c9f3d0d1ed``

  We currently set SO_INCOMING_CPU incorrectly, and testing by Ondrej
  shows that fixing the issue by setting affinities is worse than
  letting the kernel schedule threads without constraints. So we should
  not set SO_INCOMING_CPU anymore. :gl:`#4936` :gl:`!9504`

- Fix the 'rndc dumpdb' command's error reporting. ``d35f654d674``

  The 'rndc dumpdb' command wasn't reporting errors which occurred when
  starting up the database dump process by named, like, for example, a
  permission denied error for the 'dump-file' file. This has been fixed.
  Note, however, that 'rndc dumpdb' performs asynchronous writes, so
  errors can also occur during the dumping process, which will not be
  reported back to 'rndc', but which will still be logged by named.
  :gl:`#4944` :gl:`!9553`

- Fix long-running incoming transfers. ``c5cadd29d87``

  Incoming transfers that took longer than 30 seconds would stop reading
  from the TCP stream and the incoming transfer would be indefinitely
  stuck causing BIND 9 to hang during shutdown.

  This has been fixed and the `max-transfer-time-in` and `max-transfer-
  idle-in` timeouts are now honoured. :gl:`#4949` :gl:`!9536`

- Fix assertion failure when receiving DNS responses over TCP.
  ``e2058ab4619``

  When matching the received Query ID in the TCP connection, an invalid
  received Query ID can very rarely cause assertion failure. :gl:`#4952`
  :gl:`!9582`

- Don't ignore the local port number in dns_dispatch_add() for TCP.
  ``97fad455d73``

  The dns_dispatch_add() function registers the 'resp' entry in
  'disp->mgr->qids' hash table with 'resp->port' being 0, but in
  tcp_recv_success(), when looking up an entry in the hash table after a
  successfully received data the port is used, so if the local port was
  set (i.e. it was not 0) it fails to find the entry and results in an
  unexpected error.

  Set the 'resp->port' to the given local port value extracted from
  'disp->local'. :gl:`#4969` :gl:`!9581`

- Add a missing rcu_read_unlock() call on exit path. ``5db2ec07395``

  An exit path in the dns_dispatch_add() function fails to get out of
  the RCU critical section when returning early. Add the missing
  rcu_read_unlock() call. :gl:`!9564`

- Don't enable REUSEADDR on outgoing UDP sockets. ``a6692e793c3``

  The outgoing UDP sockets enabled `SO_REUSEADDR` that allows sharing of
  the UDP sockets, but with one big caveat - the socket that was opened
  the last would get all traffic.  The dispatch code would ignore the
  invalid responses in the dns_dispatch, but this could lead to
  unexpected results. :gl:`!9583`


