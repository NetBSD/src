.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

Notes for BIND 9.16.5
---------------------

New Features
~~~~~~~~~~~~

- New ``rndc`` command ``rndc dnssec -status`` shows the current DNSSEC
  policy and keys in use, the key states, and rollover status.
  [GL #1612]

Bug Fixes
~~~~~~~~~

- A race condition could occur if a TCP socket connection was closed
  while ``named`` was waiting for a recursive response. The attempt to
  send a response over the closing connection triggered an assertion
  failure in the function ``isc__nm_tcpdns_send()``. [GL #1937]

- A race condition could occur when ``named`` attempted to use a UDP
  interface that was shutting down. This triggered an assertion failure
  in ``uv__udp_finish_close()``. [GL #1938]

- Fix assertion failure when server was under load and root zone had not
  yet been loaded. [GL #1862]

- ``named`` could crash when cleaning dead nodes in ``lib/dns/rbtdb.c``
  that were being reused. [GL #1968]

- ``named`` crashed on shutdown when a new ``rndc`` connection was
  received during shutdown. This has been fixed. [GL #1747]

- The DS RRset returned by ``dns_keynode_dsset()`` was used in a
  non-thread-safe manner. This could result in an INSIST being
  triggered. [GL #1926]

- Properly handle missing ``kyua`` command so that ``make check`` does
  not fail unexpectedly when CMocka is installed, but Kyua is not.
  [GL #1950]

- The ``primary`` and ``secondary`` keywords, when used as parameters
  for ``check-names``, were not processed correctly and were being
  ignored. [GL #1949]

- ``rndc dnstap -roll <value>`` did not limit the number of saved files
  to ``<value>``. [GL !3728]

- The validator could fail to accept a properly signed RRset if an
  unsupported algorithm appeared earlier in the DNSKEY RRset than a
  supported algorithm. It could also stop if it detected a malformed
  public key. [GL #1689]

- The ``blackhole`` ACL was inadvertently disabled for client queries.
  Blocked IP addresses were not used for upstream queries but queries
  from those addresses could still be answered. [GL #1936]
