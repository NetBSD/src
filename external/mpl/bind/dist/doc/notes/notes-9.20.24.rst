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

Notes for BIND 9.20.24
----------------------

Removed Features
~~~~~~~~~~~~~~~~

- Remove ineffective TCP fallback after repeated UDP timeouts.

  When an authoritative server failed to respond to two consecutive UDP
  queries, :iscman:`named` marked the next retry as TCP but still sent
  it over UDP, producing misleading dnstap records. The ineffective
  retry path has been removed; a corrected TCP fallback will be restored
  in future BIND 9 versions. :gl:`#5529`

Feature Changes
~~~~~~~~~~~~~~~

- Fall back to TCP on receipt of a UDP response with a mismatched query ID.

  BIND used to wait silently for the correct DNS message ID on a UDP
  fetch, even after receiving a response from the expected server with
  the wrong ID, leaving room for off-path spoofing attempts to keep
  guessing within that window.  The resolver now retries the fetch over
  TCP on the first such response, and a new ``MismatchTCP`` statistics
  counter tracks how often the fallback fires. :gl:`#5449`

- Limit the number of glue records cached from a referral.

  When a delegation response contained many glue addresses per listed
  nameserver, all of them were cached without a per-nameserver bound,
  inflating resolver cache memory beyond what resolution could ever use.
  The cache now keeps at most 20 IPv4 and 20 IPv6 glue addresses per
  nameserver from a delegation. :gl:`#5701`

- Fix a resolver stall on a CNAME response to a DS query.

  A validating resolver could stall for about twelve seconds and then
  return SERVFAIL when an authoritative server answered a DS query with
  a CNAME. Such responses are now rejected promptly, so the query fails
  quickly instead of hanging. :gl:`#5878`

Bug Fixes
~~~~~~~~~

- The resolver now removes other RRsets at the same name when caching a
  CNAME.

  When an RRset is in stale cache and the authoritative server changes
  the record type to CNAME, the resolver fails to refresh the stale
  cache. This has been fixed. :gl:`#5302`

- Fix :any:`nxdomain-redirect` combined with :any:`dns64`.

  When a resolver was configured with both :any:`nxdomain-redirect` and
  :any:`dns64` in the same view, an AAAA query for a nonexistent name
  could abort :iscman:`named`. The combination failed whenever the
  redirect zone held A records but no AAAA records.  The server now
  serves the empty AAAA response from the redirect zone as-is, instead
  of attempting DNS64 synthesis on top of it. :gl:`#5789`

- Fix DNS64 owner case after DNAME restart.

  When BIND 9 was configured to use DNS64 and encountered a DNAME
  redirect, it could end up using freed memory for the DNS response
  owner name. This caused the response to contain corrupted data. This
  fix ensures the correct owner name is used when constructing the
  synthesized response after a DNAME redirect. :gl:`#5934`

- Clear REDIRECT flag when it isn't needed.

  When :any:`nxdomain-redirect` is in use, and a recursive query is used
  to get the redirected answer, a flag is set to distinguish it from a
  normal recursive response. Previously, that flag was left set
  afterward, which could trigger an assertion if a normal recursive
  query was sent later on behalf of the same client: for example,
  because the :any:`filter-aaaa` plugin was in use.  This has been
  fixed. :gl:`#5936`

- Disable output escaping in ``bind9.xsl``.

  The statistics charts were not displaying on some browsers. This has
  been fixed. :gl:`#5990`

- Fix crash on badly configured secondary signer.

  A badly configured secondary signer that was missing the ``file``
  entry caused the server to crash, rather than to reject the
  configuration. This has been fixed. :gl:`#5993`

- Fix a possible crash on concurrent TKEY DELETE for the same key.

  On a server configured with :any:`tkey-gssapi-keytab`, an
  authenticated peer could crash :iscman:`named` by sending two TKEY
  DELETE requests for the same dynamic key in rapid succession.  This
  has been fixed. :gl:`#6001`

- Reject RRSIG records covering meta-types.

  A recursive resolver could accept and cache an RRSIG record whose
  Type-Covered field named a meta-type (ANY, AXFR, IXFR, MAILA, MAILB),
  even though no real RRset of those types ever exists. Such records are
  now rejected by the DNS message parser. :gl:`#6002`
