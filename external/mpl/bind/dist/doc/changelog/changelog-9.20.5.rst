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

BIND 9.20.5
-----------

Security Fixes
~~~~~~~~~~~~~~

- [CVE-2024-12705] DNS-over-HTTP(s) flooding fixes. ``51900adf29c``

  Fix DNS-over-HTTP(S) implementation issues that arise under heavy
  query load. Optimize resource usage for :iscman:`named` instances that
  accept queries over DNS-over-HTTP(S).

  Previously, :iscman:`named` would process all incoming HTTP/2 data at
  once, which could overwhelm the server, especially when dealing with
  clients that send requests but don't wait for responses. That has been
  fixed. Now, :iscman:`named` handles HTTP/2 data in smaller chunks and
  throttles reading until the remote side reads the response data. It
  also throttles clients that send too many requests at once.

  Additionally, :iscman:`named` now carefully processes data sent by
  some clients, which can be considered "flooding." It logs these
  clients and drops connections from them. :gl:`#4795`

  In some cases, :iscman:`named` could leave DNS-over-HTTP(S)
  connections in the `CLOSE_WAIT` state indefinitely. That also has been
  fixed. ISC would like to thank JF Billaud for thoroughly investigating
  the issue and verifying the fix. :gl:`#5083` :gl:`#4795` :gl:`#5083`

- [CVE-2024-11187] Limit the additional processing for large RDATA sets.
  ``4d3d17c344f``

  When answering queries, don't add data to the additional section if
  the answer has more than 13 names in the RDATA. This limits the number
  of lookups into the database(s) during a single client query, reducing
  query processing load. :gl:`#5034`

New Features
~~~~~~~~~~~~

- Add Extended DNS Error Code 22 - No Reachable Authority.
  ``ee77a192091``

  When the resolver is trying to query an authority server and
  eventually timed out, a SERVFAIL answer is given to the client. Add
  the Extended DNS Error Code 22 - No Reachable Authority to the
  response. :gl:`#2268` :gl:`!9814`

- Add a new option to configure the maximum number of outgoing queries
  per client request. ``844a5310532``

  The configuration option 'max-query-count' sets how many outgoing
  queries per client request is allowed. The existing
  'max-recursion-queries' is the number of permissible queries for a
  single name and is reset on every CNAME redirection. This new option
  is a global limit on the client request. The default is 200.

  This allows us to send a bit more queries while looking up a single
  name. The default for 'max-recursion-queries' is changed from 32 to
  50. :gl:`#4980`  :gl:`#4921` :gl:`!9832`

Removed Features
~~~~~~~~~~~~~~~~

- Drop single-use RETERR macro. ``87f70696c87``

  If the RETERR define is only used once in a file, just drop the macro.
  :gl:`!9885`

Feature Changes
~~~~~~~~~~~~~~~

- Update picohttpparser.{c,h} with upstream repository. ``3c9657a3f48``

  :gl:`#4485` :gl:`!9863`

- The configuration clauses parental-agents and primaries are renamed to
  remote-servers. ``b483cd4638c``

  The top blocks 'primaries' and 'parental-agents' are no longer
  preferred and should be renamed to 'remote-servers'. The zone
  statements 'parental-agents' and 'primaries' are still used, and may
  refer to any 'remote-servers' top block. :gl:`#4544` :gl:`!9911`

- Add none parameter to query-source and query-source-v6 to disable IPv4
  or IPv6 upstream queries. ``e260eb39c56``

  Add a none parameter to named configuration option `query-source`
  (respectively `query-source-v6`) which forbid usage of IPv4
  (respectively IPv6) addresses when named is doing an upstream query.
  :gl:`#4981` Turning-off upstream IPv6 queries while still listening to
  downstream queries on IPv6. :gl:`!9727` :gl:`!9775`

- Optimize memory layout of core structs. ``67fa22a7746``

  Reduce memory footprint by: - Reordering struct fields to minimize
  padding. - Using exact-sized atomic types instead of
  `*_least`/`*_fast` variants - Downsizing integer fields where possible

  Affected structs: - dns_name_t - dns_slabheader_t  - dns_rdata_t -
  qpcnode_t - qpznode_t :gl:`#5022` :gl:`!9793`

- Revert "Fix NSEC3 closest encloser lookup for names with empty
  non-terminals" ``993cb761489``

  Revert the fix for #4950 for 9.20.

  This reverts MR !9438.

  History: A performance improvement for NSEC3 closest encloser lookups
  (#4460) was introduced (in MR !9436) and backported to 9.20 (MR !9438)
  and to 9.18 in (MR !9439). It was released in 9.18.30 (and 9.20.2 and
  9.21.1).

  There was a bug in the code (#4950), so we reverted the change in
  !9611, !9613 and !9614.

  Then a new attempt was merged in main (MR !9610) and backported to
  9.20 (MR !9631) and 9.18 (MR !9632). The latter should not have been
  backported and was reverted in !9689.

  We now also revert the fix for 9.20 :gl:`#5108` :gl:`!9947`

- Add TLS SNI extension to all outgoing TLS connections. ``b14148ac897``

  :gl:`!9933`

- Remove unused maxquerycount. ``d61bfeb91e0``

  Related to #4980 :gl:`!9853`

- Use query counters in validator code. ``d91835160a2``

  Commit af7db8951364a89c468eda1535efb3f53adc2c1f as part of #4141 was
  supposed to apply the 'max-recursion-queries' quota to validator
  queries, but the counter was never actually passed on to
  'dns_resolver_createfetch()'. This has been fixed, and the global
  query counter ('max-query-count', per client request) is now also
  added.

  Related to #4980 :gl:`!9866`

Bug Fixes
~~~~~~~~~

- Fix nsupdate hang when processing a large update. ``4ca7a5d6011``

  To mitigate DNS flood attacks over a single TCP connection, we
  throttle the connection when the other side does not read the data.
  Throttling should only occur on server-side sockets, but erroneously
  also happened for nsupdate, which acts as a client. When nsupdate
  started throttling the connection, it never attempts to read again.
  This has been fixed.   :gl:`#4910` :gl:`!9834`

- Lock and attach when returning zone stats. ``79e6519168e``

  When returning zone statistics counters, the statistics sets are now
  attached while the zone is locked.  This addresses Coverity warnings
  CID 468720, 468728 and 468729. :gl:`#4934` :gl:`!9843`

- Fix possible assertion failure when reloading server while processing
  updates. ``41af766cd08``

  :gl:`#5006` :gl:`!9820`

- Preserve cache across reconfig when using attach-cache.
  ``826dfa006e2``

  When the `attach-cache` option is used in the `options` block with an
  arbitrary name, it causes all views to use the same cache. Previously,
  this configuration caused the cache to be deleted and a new cache
  created every time the server was reconfigured. This has been fixed.
  :gl:`#5061` :gl:`!9862`

- Resolve the spurious drops in performance due GLUE cache.
  ``eb3c66304f3``

  For performance reasons, the returned GLUE records are cached on the
  first use.  The current implementation could randomly cause a
  performance drop and increased memory use.  This has been fixed.
  :gl:`#5064` :gl:`!9918`

- Fix dnssec-signzone signing non-DNSKEY RRsets with revoked keys.
  ``c577c3b544d``

  `dnssec-signzone` was using revoked keys for signing RRsets other than
  DNSKEY.  This has been corrected. :gl:`#5070` :gl:`!9840`

- Revert "Lock and attach when returning zone stats" ``d954d9c20b9``

  :gl:`#5082` :gl:`!9860`

- Unknown directive in resolv.conf not handled properly. ``7738fd28c91``

  The line after an unknown directive in resolv.conf could accidentally
  be skipped, potentially affecting dig, host, nslookup, nsupdate, or
  delv. This has been fixed. :gl:`#5084` :gl:`!9877`

- Fix response policy zones and catalog zones with an $INCLUDE statement
  defined. ``cc0cbbe697c``

  Response policy zones (RPZ) and catalog zones were not working
  correctly if they had an $INCLUDE statement defined. This has been
  fixed. :gl:`#5111` :gl:`!9941`

- Finalize removal of memory debug flags size and mctx. ``31918336e8a``

  Commit 4b3d0c66009d30f5c0bc12ee128fc59f1d853f44 has removed them, but
  did not remove few traces in documentation and help. Remove them from
  remaining places. :gl:`!9842`

- Fix m4 macro in configure.ac. ``ae739c80ccb``

  :gl:`!9813`

- Mark loop as shuttingdown earlier in shutdown_cb. ``fed5e55e339``

  :gl:`!9891`

- Use CMM_{STORE,LOAD}_SHARED to store/load glue in gluelist.
  ``fa7443d3fd2``

  ThreadSanitizer has trouble understanding that gluelist->glue is
  constant after it is assigned to the slabheader with cmpxchg.  Help
  ThreadSanitizer to understand the code by using CMM_STORE_SHARED and
  CMM_LOAD_SHARED on gluelist->glue. :gl:`!9936`


