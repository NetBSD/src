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

Notes for BIND 9.20.7
---------------------

New Features
~~~~~~~~~~~~

- Implement the :any:`min-transfer-rate-in` configuration option.

  A new option :any:`min-transfer-rate-in` has been added
  to the view and zone configurations. It can abort incoming zone
  transfers that run very slowly due to network-related issues, for
  example. The default value is 10240 bytes in five minutes.
  :gl:`#3914`

- Add HTTPS record query to :iscman:`host` command line tool.

  The :iscman:`host` command was extended to also query for the HTTPS RR
  type by default.

- Implement :any:`sig0key-checks-limit` and :any:`sig0message-checks-limit`.

  Previously, a hard-coded limitation of a maximum of two key or message
  verification checks was introduced when checking a message's ``SIG(0)``
  signature, to protect against possible DoS
  attacks. Two as a maximum was chosen so that more than a
  single key should only be required during key rotations, and in that
  case two keys are enough. It later became apparent that there are
  other use cases where even more keys are required; see the related GitLab issue for examples.

  This change introduces two new configuration options for the views:
  :any:`sig0key-checks-limit` and :any:`sig0message-checks-limit`. They define
  how many keys can be checked to find a matching key, and
  how many message verifications are allowed to take place once a
  matching key has been found. The former provides
  slightly less "expensive" key parsing operations and defaults to
  16. The latter protects against expensive
  cryptographic operations when there are keys with colliding tags and
  algorithm numbers; the default is 2. :gl:`#5050`

Bug Fixes
~~~~~~~~~


- Fix :any:`dual-stack-servers` configuration option.

  The :any:`dual-stack-servers` configuration option was not working as
  expected; the specified servers were not being used when they should
  have been, leading to resolution failures. This has been fixed.
  :gl:`#5019`

- Fix a data race causing a permanent active client increase.

  Previously, a data race could cause a newly created fetch context for
  a new client to be used before it had been fully initialized, which
  would cause the query to become stuck; queries for the same data would
  be either paused indefinitely or dropped because of the
  :any:`clients-per-query` limit. This has been fixed. :gl:`#5053`

- Fix deferred validation of unsigned DS and DNSKEY records.

  When processing a query with the "checking disabled" bit set (CD=1),
  :iscman:`named` stores the invalidated result in the cache, marked "pending".
  When the same query is sent with CD=0, the cached data is validated
  and either accepted as an answer, or ejected from the cache as
  invalid. This deferred validation was not attempted for DS and DNSKEY
  records if they had no cached signatures, causing spurious validation
  failures. The deferred validation is now completed in this scenario.

  Also, if deferred validation fails, the data is now re-queried to find
  out whether the zone has been corrected since the invalid data was
  cached. :gl:`#5066`

- Fix RPZ race condition during a reconfiguration.

  With RPZ in use, :iscman:`named` could terminate unexpectedly because of a
  race condition when a reconfiguration command was received using
  :iscman:`rndc`. This has been fixed. :gl:`#5146`

- "CNAME and other data check" not applied to all types.

  An incorrect optimization caused "CNAME and other data" errors not to
  be detected if certain types were at the same node as a CNAME.  This
  has been fixed. :gl:`#5150`

- Relax private DNSKEY and RRSIG constraints.

  DNSKEY, KEY, RRSIG, and SIG constraints have been relaxed to allow
  empty key and signature material after the algorithm identifier for
  PRIVATEOID and PRIVATEDNS. It is arguable whether this falls within
  the expected use of these types, as no key material is shared and the
  signatures are ineffective, but these are private algorithms and they
  can be totally insecure. :gl:`#5167`

- Remove NSEC/DS/NSEC3 RRSIG check from ``dns_message_parse()``.

  Previously, when parsing responses, :iscman:`named` incorrectly rejected
  responses without matching RRSIG records for NSEC/DS/NSEC3 records in
  the authority section. This rejection, if appropriate, should have
  been left for the validator to determine and has been fixed.
  :gl:`#5185`

- Fix TTL issue with ANY queries processed through RPZ "passthru".

  Answers to an "ANY" query which were processed by the RPZ "passthru"
  policy had the response-policy's ``max-policy-ttl`` value unexpectedly
  applied. This has been fixed. :gl:`#5187`

- :iscman:`dnssec-signzone` needs to check for a NULL key when setting offline.

  :iscman:`dnssec-signzone` could dereference a NULL key pointer when resigning
  a zone.  This has been fixed. :gl:`#5192`


- Fix a bug in the statistics channel when querying zone transfer
  information.

  When querying zone transfer information from the statistics channel,
  there was a rare possibility that :iscman:`named` could terminate unexpectedly
  if a zone transfer was in a state when transferring from all the
  available primary servers had failed earlier. This has been fixed.
  :gl:`#5198`

- Fix assertion failure when dumping recursing clients.

  Previously, if a new counter was added to the hash table while dumping
  recursing clients via the :option:`rndc recursing` command, and
  :any:`fetches-per-zone` was enabled, an assertion failure could occur. This
  has been fixed. :gl:`#5200`

- Dump the active resolver fetches from ``dns_resolver_dumpfetches()``

  Previously, active resolver fetches were only dumped when the
  :any:`fetches-per-zone` configuration option was enabled. Now, active
  resolver fetches are dumped along with the number of
  :any:`clients-per-query` counters per resolver fetch.


