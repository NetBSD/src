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

BIND 9.20.24
------------

Security Fixes
~~~~~~~~~~~~~~

- Fix DNS64 owner case after DNAME restart. ``4de2229364``

  When BIND 9 is configured to use DNS64 and encounters a DNAME
  redirect, it could end up using freed memory for the DNS response
  owner name. This caused the response to contain corrupted data. This
  fix ensures the correct owner name is used when constructing the
  synthesized response after a DNAME redirect.

  ISC thanks Qifan Zhang of Palo Alto Networks for reporting the issue.
  :gl:`#5934`

New Features
~~~~~~~~~~~~

- Enable PR-Agent reviews on merge requests. ``46e4c236a3``

  Adds a CI job that runs PR-Agent against each merge request opened
  from the canonical repository, posting an automated review and
  code-improvement suggestions as MR comments. The job is gated to
  same-project source branches so the OpenAI key and personal access
  token are not exposed to fork pipelines. :gl:`!12034`

Removed Features
~~~~~~~~~~~~~~~~

- Remove ineffective TCP fallback after repeated UDP timeouts.
  ``eb13adcb47``

  When an authoritative server failed to respond to two consecutive UDP
  queries, named marked the next retry as TCP but still sent it over
  UDP, producing misleading dnstap records. The ineffective retry path
  has been removed; a corrected TCP fallback will be restored in future
  BIND 9 versions. :gl:`#5529` :gl:`!12049`

- Remove useless PR-Agent jobs. ``8851b279d0``

  The experiment was a failure, the PR-Agent doesn't send a full context
  to the AI Agents and the results are abysmal because of that.
  :gl:`!12120`

Feature Changes
~~~~~~~~~~~~~~~

- Fall back to TCP on a UDP response with a mismatched query id.
  ``b2367aaea2``

  BIND used to wait silently for the correct DNS message id on a UDP
  fetch even after receiving a response from the expected server with
  the wrong id, leaving room for off-path spoofing attempts to keep
  guessing within that window.  The resolver now retries the fetch over
  TCP on the first such response, and a new MismatchTCP statistics
  counter tracks how often the fallback fires. :gl:`#5449` :gl:`!12025`

- Limit the number of glue records cached from a referral.
  ``eb401f6b92``

  When a delegation response contained many glue addresses per listed
  nameserver, all of them were cached without a per-nameserver bound,
  inflating resolver cache memory beyond what resolution could ever use.
  The cache now keeps at most 20 IPv4 and 20 IPv6 glue addresses per
  nameserver from a delegation. :gl:`#5701` :gl:`!11972`

- Fix a resolver stall on a CNAME response to a DS query. ``1407f48670``

  A validating resolver could stall for about twelve seconds and then
  return SERVFAIL when an authoritative server answered a DS query with
  a CNAME. Such responses are now rejected promptly, so the query fails
  fast instead of hanging. :gl:`#5878` :gl:`!12147`

- Named could crash on concurrent TKEY DELETE for the same key.
  ``dd02abde67``

  On a server configured with tkey-gssapi-keytab (or
  tkey-gssapi-credential), an authenticated peer could crash named by
  sending two TKEY DELETE requests for the same dynamic key in rapid
  succession.  This has been fixed. :gl:`#6001` :gl:`!12042`

Bug Fixes
~~~~~~~~~

- The resolver now removes other RRsets at the same name when caching a
  CNAME. ``1547447491``

  When an RRset is in stale cache, and the authoritative server changes
  the record type to CNAME, the resolver fails to refresh the stale
  cache. This has been fixed. :gl:`#5302` :gl:`!12040`

- Fix nxdomain-redirect combined with dns64. ``95274e9455``

  When a resolver was configured with both `nxdomain-redirect` and
  `dns64` in the same view, an AAAA query for a nonexistent name could
  abort `named`. The combination failed whenever the redirect zone held
  A records but no AAAA records.  The server now serves the empty AAAA
  response from the redirect zone as-is, instead of attempting DNS64
  synthesis on top of it. :gl:`#5789` :gl:`!12123`

- Clear REDIRECT flag when it isn't needed. ``86bb27060c``

  When `nxdomain-redirect` is in use, and a recursive query is used to
  get the redirected answer, a flag is set to distinguish it from a
  normal recursive response. Previously, that flag was left set
  afterward, which could trigger an assertion if a normal recursive
  query was sent later on behalf of the same client: for example,
  because the `filter-aaaa` plugin was in use.  This has been fixed.
  :gl:`#5936` :gl:`!12076`

- Fix data race during rndc dumpdb or zone load. ``947e5c7983``

  'rndc dumpdb' against a server with zones, and async zone load, had a
  timing window where the operation's completion could fire before the
  server had finished registering the operation, occasionally leading to
  a possible crash.  The completion is now delivered after the
  registration is in place. :gl:`#5952` :gl:`!12021`

- Bound memory use during incoming zone transfers. ``5d7f241fdf``

  During an incoming zone transfer, an optimization could let the batch
  of pending records grow without bound for a large zone, raising memory
  usage. It gave no measurable performance benefit, so it has been
  removed. :gl:`#5958` :gl:`!12142`

- Disable output escaping in bind9.xsl. ``b514e663eb``

  The statistics charts where not displaying on some browsers. This has
  been fixed. :gl:`#5990` :gl:`!12019`

- Fix crash on badly configured secondary signer. ``edc1ef084f``

  A badly configured secondary signer that was missing the 'file' entry
  caused the server to crash, rather than to reject the configuration.
  This has been fixed. :gl:`#5993` :gl:`!12112`

- Avoid named assertion failure during parent-NS lookups when none
  exist. ``5c0c4786dd``

  Configuring the root zone as a signed primary with parental agents (or
  with notify-on-cds-changes) caused named to exit on an internal
  assertion as soon as the DS-publication machinery tried to look up the
  parent NS RRset — the root has no parent. The lookup is now
  short-circuited cleanly.

  Similar, a zone with no NS records in the parent caused named to exit
  in the same way. :gl:`#5910` :gl:`#5996` :gl:`!12053`

- Reject RRSIG records covering meta-types. ``7517e39504``

  A recursive resolver could accept and cache an RRSIG record whose
  Type-Covered field names a meta-type (ANY, AXFR, IXFR, MAILA, MAILB),
  even though no real RRset of those types ever exists. Such records are
  now rejected by the DNS message parser. :gl:`#6002` :gl:`!12051`

- Validate nsec3hash arguments instead of relying on atoi()
  ``a59080c053``

  The nsec3hash tool parsed its algorithm, flags, and iterations
  arguments with atoi(), then range-checked the result. For values that
  overflow int during digit-by-digit accumulation, atoi() is undefined;
  in practice on musl libc the modular wrap leaves n == 0, which
  silently passes the "iterations > 0xffffU" check. On Alpine Linux this
  made nsec3hash succeed with iterations treated as 0 for inputs like
  4294967296 (2^32).

  The latent bug only surfaced when the recent image rebuild pulled in
  Hypothesis 6.152.9 (2026-05-19), which unified the distribution used
  for bounded and unbounded integers() strategies. The new smoother
  distribution explores the 2^32 boundary on unbounded ranges like
  integers(min_value=65536); earlier versions did not reach there, so
  test_nsec3hash_too_many_iterations only started failing on Alpine
  after the image refresh.

  Replace the three atoi() calls with isc_parse_uint8 /
  isc_parse_uint16, which uniformly reject overflow, trailing garbage,
  leading sign, and non-numeric input across libc implementations. As a
  side effect, error messages now include the offending argument and a
  specific reason ("out of range" vs "not a valid number").

  Assisted-by: Claude:claude-opus-4-7 :gl:`#6013` :gl:`!12074`

- Fix wrong variable in named_server_sync() log message. ``09f9889ef6``

  named_server_sync() logged isc_result_totext(result) but returns
  tresult. The loop accumulates errors into tresult, so result only
  holds the last iteration's value. If the last view succeeded but an
  earlier one failed, the log would incorrectly say "success".
  :gl:`!12156`

- Refine resolver fetch loop detection. ``787faa02a7``

  The resolver's fetch loop detection now triggers only when a new fetch
  would join an already in-flight fetch that is also one of its own
  ancestors, which is the actual loop condition.  Previously the check
  ran against the original request before the fetch was set up.
  :gl:`!12146`


