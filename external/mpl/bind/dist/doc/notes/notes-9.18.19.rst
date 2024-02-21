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

Notes for BIND 9.18.19
----------------------

Security Fixes
~~~~~~~~~~~~~~

- Previously, sending a specially crafted message over the control
  channel could cause the packet-parsing code to run out of available
  stack memory, causing :iscman:`named` to terminate unexpectedly.
  This has been fixed. :cve:`2023-3341`

  ISC would like to thank Eric Sesterhenn from X41 D-Sec GmbH for
  bringing this vulnerability to our attention. :gl:`#4152`

- A flaw in the networking code handling DNS-over-TLS queries could
  cause :iscman:`named` to terminate unexpectedly due to an assertion
  failure under significant DNS-over-TLS query load. This has been
  fixed. :cve:`2023-4236`

  ISC would like to thank Robert Story from USC/ISI Root Server
  Operations for bringing this vulnerability to our attention.
  :gl:`#4242`

Removed Features
~~~~~~~~~~~~~~~~

- The :any:`dnssec-must-be-secure` option has been deprecated and will
  be removed in a future release. :gl:`#4263`

Feature Changes
~~~~~~~~~~~~~~~

- If the ``server`` command is specified, :iscman:`nsupdate` now honors
  the :option:`nsupdate -v` option for SOA queries by sending both the
  UPDATE request and the initial query over TCP. :gl:`#1181`

Bug Fixes
~~~~~~~~~

- The value of the If-Modified-Since header in the statistics channel
  was not being correctly validated for its length, potentially allowing
  an authorized user to trigger a buffer overflow. Ensuring the
  statistics channel is configured correctly to grant access exclusively
  to authorized users is essential (see the :any:`statistics-channels`
  block definition and usage section). :gl:`#4124`

  This issue was reported independently by Eric Sesterhenn of X41 D-Sec
  GmbH and Cameron Whitehead.

- The Content-Length header in the statistics channel was lacking proper
  bounds checking. A negative or excessively large value could
  potentially trigger an integer overflow and result in an assertion
  failure. :gl:`#4125`

  This issue was reported by Eric Sesterhenn of X41 D-Sec GmbH.

- Several memory leaks caused by not clearing the OpenSSL error stack
  were fixed. :gl:`#4159`

  This issue was reported by Eric Sesterhenn of X41 D-Sec GmbH.

- The introduction of ``krb5-subdomain-self-rhs`` and
  ``ms-subdomain-self-rhs`` UPDATE policies accidentally caused
  :iscman:`named` to return SERVFAIL responses to deletion requests for
  non-existent PTR and SRV records. This has been fixed. :gl:`#4280`

- The :any:`stale-refresh-time` feature was mistakenly disabled when the
  server cache was flushed by :option:`rndc flush`. This has been fixed.
  :gl:`#4278`

- BIND's memory consumption has been improved by implementing dedicated
  jemalloc memory arenas for sending buffers. This optimization ensures
  that memory usage is more efficient and better manages the return of
  memory pages to the operating system. :gl:`#4038`

- Previously, partial writes in the TLS DNS code were not accounted for
  correctly, which could have led to DNS message corruption. This has
  been fixed. :gl:`#4255`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
