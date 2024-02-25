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

Notes for BIND 9.18.7
---------------------

Security Fixes
~~~~~~~~~~~~~~

- Previously, there was no limit to the number of database lookups
  performed while processing large delegations, which could be abused to
  severely impact the performance of :iscman:`named` running as a
  recursive resolver. This has been fixed. :cve:`2022-2795`

  ISC would like to thank Yehuda Afek from Tel-Aviv University and Anat
  Bremler-Barr & Shani Stajnrod from Reichman University for bringing
  this vulnerability to our attention. :gl:`#3394`

- When an HTTP connection was reused to request statistics from the
  stats channel, the content length of successive responses could grow
  in size past the end of the allocated buffer. This has been fixed.
  :cve:`2022-2881` :gl:`#3493`

- Memory leaks in code handling Diffie-Hellman (DH) keys were fixed that
  could be externally triggered, when using TKEY records in DH mode with
  OpenSSL 3.0.0 and later versions. :cve:`2022-2906` :gl:`#3491`

- :iscman:`named` running as a resolver with the
  :any:`stale-answer-client-timeout` option set to ``0`` could crash
  with an assertion failure, when there was a stale CNAME in the cache
  for the incoming query. This has been fixed. :cve:`2022-3080`
  :gl:`#3517`

- Memory leaks were fixed that could be externally triggered in the
  DNSSEC verification code for the EdDSA algorithm. :cve:`2022-38178`
  :gl:`#3487`

Feature Changes
~~~~~~~~~~~~~~~

- Response Rate Limiting (RRL) code now treats all QNAMEs that are
  subject to wildcard processing within a given zone as the same name,
  to prevent circumventing the limits enforced by RRL. :gl:`#3459`

- Zones using :any:`dnssec-policy` now require dynamic DNS or
  :any:`inline-signing` to be configured explicitly. :gl:`#3381`

- When reconfiguring :any:`dnssec-policy` from using NSEC with an
  NSEC-only DNSKEY algorithm (e.g. RSASHA1) to a policy that uses NSEC3,
  BIND 9 no longer fails to sign the zone; instead, it keeps using NSEC
  until the offending DNSKEY records have been removed from the zone,
  then switches to using NSEC3. :gl:`#3486`

- A backward-compatible approach was implemented for encoding
  internationalized domain names (IDN) in :iscman:`dig` and converting
  the domain to IDNA2008 form; if that fails, BIND tries an IDNA2003
  conversion. :gl:`#3485`

Bug Fixes
~~~~~~~~~

- A serve-stale bug was fixed, where BIND would try to return stale data
  from cache for lookups that received duplicate queries or queries that
  would be dropped. This bug resulted in premature SERVFAIL responses,
  and has now been resolved. :gl:`#2982`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
