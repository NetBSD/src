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

BIND 9.20.20
------------

Feature Changes
~~~~~~~~~~~~~~~

- Record query time for all dnstap responses. ``f4fdcee03f1``

  Not all DNS responses had the query time set in their corresponding
  dnstap messages. This has been fixed. :gl:`#3695` :gl:`!11534`

- Implement Fisher-Yates shuffle for nameserver selection.
  ``dd453590a0e``

  Replace the two-pass "random start index and wrap around" logic in
  fctx_getaddresses_nameservers() with a statistically sound partial
  Fisher-Yates shuffle.

  The previous implementation picked a random starting node and did two
  passes over the linked list to find query candidates. The new logic
  introduces fctx_getaddresses_nsorder() to perform an in-place
  randomization of indices into a bounded, stack-allocated lookup array
  (nsorder) representing the "winning" fetch slots.

  The nameserver dataset is now traversed in exactly one sequential
  pass: 1. Every nameserver is evaluated for local cached data. 2. If
  the current nameserver's sequential index exists in the randomized
  nsorder array, it is permitted to launch an outgoing network fetch. 3.
  If not, it is restricted to local lookups via DNS_ADBFIND_NOFETCH.

  This guarantees a fair random distribution for outbound queries while
  maximizing local cache hits, entirely within O(1) memory and without
  the overhead of linked-list pointer shuffling or dynamic allocation.
  :gl:`#5695` :gl:`!11606`

- Invalid NSEC3 can cause OOB read of the isdelegation() stack.
  ``e6f234169e2``

  When .next_length is longer than NSEC3_MAX_HASH_LENGTH, it causes a
  harmless out-of-bound read of the isdelegation() stack.  This has been
  fixed. :gl:`#5749` :gl:`!11594`

- Optimize the TCP source port selection on Linux. ``d4426f85b36``

  Enable a socket option on the outgoing TCP sockets to allow faster
  selection of the source <address,port> tuple for different destination
  <address,port> tuples when nearing over 70-80% of the source port
  utilization. :gl:`!11573`

Bug Fixes
~~~~~~~~~

- Fix errors when retrying over TCP in notify_send_toaddr.
  ``a1232333196``

  If the source address is not available do not attempt to retry over
  TCP otherwise clear the TSIG key from the message prior to retrying.
  :gl:`#5457` :gl:`!11567`

- Fetch loop detection improvements. ``892c3e78926``

  Fixes a case where an in-domain NS with an expired glue would fail to
  resolve.

  Let's consider the following parent-side delegation (both for
  `foo.example.` and `dnshost.example.`

  ``` foo.example.            3600    NS      ns.dnshost.example.
  dnshost.example.        3600    NS      ns.dnshost.example.
  ns.dnshost.example.     3600    A       1.2.3.4 ```      Then the
  child-side of `dnshost.example.`:

  ```     dnshost.example.        300     NS      ns.dnshost.example.
  ns.dnshost.example.     300     A       1.2.3.4 ```      And then the
  child-side of `foo.example.`:

  ``` foo.example             3600    NS      ns.dnshost.example.
  a.foo.example           300     A       5.6.7.8 ```

  While there is a zone misconfiguration (the TTL of the delegation and
  glue doesn't match in the parent and the child), it is possible to
  resolve `a.foo.example` on a cold-cache resolver. However, after the
  `ns.dnshost.example.` glue expires, the resolution would have failed
  with a "fetch loop detected" error. This is now fixed. :gl:`#5588`
  :gl:`!11547`

- Remove deterministic selection of nameserver. ``c6c6e490fd8``

  When selecting nameserver addresses to be looked up we where always
  selecting them in dnssec name order from the start of the nameserver
  rrset.  This could lead to resolution failure despite there being
  address that could be resolved for the other names.  Use a random
  starting point when selecting which names to lookup. :gl:`#5695`
  :gl:`#5745` :gl:`!11600`

- DNSTAP wasn't logging forwarded queries correctly. ``0a5922bcf7a``

  :gl:`#5724` :gl:`!11555`

- Fix read UAF in BIND9 dns_client_resolve() via DNAME Response.
  ``c0c4bf526a1``

  An attacker controlling a malicious DNS server returns a DNAME record,
  and the we stores a pointer to resp->foundname, frees the response
  structure, then uses the dangling pointer in dns_name_fullcompare()
  possibly causing invalid match.  Only the `delv`is affected.  This has
  been fixed. :gl:`#5728` :gl:`!11571`

- Clear serve-stale flags when following the CNAME chains.
  ``68fb2312948``

  A stale answer could have been served in case of multiple upstream
  failures when following the CNAME chains.  This has been fixed.
  :gl:`#5751` :gl:`!11583`

- Fail DNSKEY validation when supported but invalid DS is found.
  ``2e1971873a1``

  A regression was introduced when adding the EDE code for unsupported
  DNSKEY and DS algorithms.  When the parent has both supported and
  unsupported algorithm in the DS record, the validator would treat the
  supported DS algorithm as insecure when validating DNSKEY records
  instead of BOGUS.  This has not security impact as the rest of the
  child zone correctly ends with BOGUS status, but it is incorrect and
  thus the regression has been fixed. :gl:`#5757` :gl:`!11590`

- Importing invalid SKR file might corrupt stack memory. ``9869a14ce3a``

  If an BIND 9 administrator imports an invalid SKR file, local stack in
  the import function might overflow.  This could lead to a memory
  corruption on the stack and ultimately server crash. This has been
  fixed.

  ISC would like to thank mcsky23 for bringing this bug to our
  attention. :gl:`#5758` :gl:`!11598`

- Do not update the case on unchanged rdatasets. ``8931f82dc8b``

  Fix assertion failure on unchanged rdataset during IXFR. :gl:`#5759`
  :gl:`!11587`

- Return FORMERR for ECS family 0. ``8ac316bf0f6``

  RFC 7871 only defines family 1 (IPv4) and 2 (IPv6). Additionally it
  requires FORMERR to be returned for all unknown families. :gl:`!11565`


