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

BIND 9.20.22
------------

Security Fixes
~~~~~~~~~~~~~~

- Fix crash when reconfiguring zone update policy during active updates.
  ``ee7832ae583``

  Fixed a crash that could occur when running rndc reconfig to change a
  zone's update policy (e.g., from allow-update to update-policy) while
  DNS UPDATE requests were being processed for that zone.

  ISC would like to thank Vitaly Simonovich for bringing this issue to
  our attention. :gl:`#5817` :gl:`!11738`

New Features
~~~~~~~~~~~~

- Add MOVE_OWNERSHIP() macro for transferring pointer ownership.
  ``13a656f79aa``

  A helper macro that returns the current value of a pointer and sets it
  to NULL in one expression, useful for transferring ownership in
  designated initializers. :gl:`!11736`

Feature Changes
~~~~~~~~~~~~~~~

- Exclude named.args.j2 and system test README files from license header
  checks. ``d65e3922bbb``

  Exclude named.args.j2 files from license header checks so named.args
  can be generated from Jinja templates. Also exclude system test README
  files from the license header checks. :gl:`!11696`

- Skip cache flush ordering on NTA expiry. ``5f97f5b0501``

  dns_view_flushnode() was called in the delete_expired() async
  callback, which runs after the query that detected the NTA expiry.
  This created a race: the query would proceed with stale cached data
  from the NTA period before the flush had a chance to run, resulting in
  transient SERVFAIL with EDE 22 (No Reachable Authority).

  Skip dns_view_flushnode() in the older branches as the solutions for
  older branches are too complicated and this was not a critical bug.

  Also simplify the expiry comparison in delete_expired() to a direct
  pointer comparison (nta == pval) instead of comparing expiry
  timestamps. :gl:`!11730`

- Use underscore for system test names. ``d270709b499``

  Change the convention for system test directory names to always use an
  underscore rather than a hyphen. Names using underscore are valid
  python package names and can be used with standard `import` facilities
  in python, which allows easier code reuse. :gl:`!11711`

Bug Fixes
~~~~~~~~~

- Fix intermittent named crashes during asynchronous zone operations.
  ``ac042af5766``

  Asynchronous zone loading and dumping operations occasionally
  dispatched tasks to the wrong internal event loop. This threading
  violation triggered internal safety assertions that abruptly
  terminated named. Strict loop affinity is now enforced for these
  tasks, ensuring they execute on their designated threads and
  preventing the crashes. :gl:`#4882` :gl:`!11684`

- Count temporal problems with DNSSEC validation as attempts.
  ``e4399fc6b26``

  After KeyTrap, the temporal DNSSEC were originally hard errors that
  caused validation failures even if the records had another valid
  signature.  This has been changed and the RRSIGs outside of the
  inception and expiration time are not counted as hard errors.
  However, these errors are not even counted as validation attempts, so
  excessive number of expired RRSIGs would cause some non-cryptograhic
  extra work for the validator.  This has been fixed and the temporal
  errors are correctly counted as validation attempts. :gl:`#5760`
  :gl:`!11763`

- Clear errno correctly. ``0de8a660117``

  Zero errno before calling strtol. :gl:`#5773` :gl:`!11703`

- Fix a possible deadlock in RPZ processing. ``a2bedda1321``

  The :iscman:`named` process could hang when processing a maliciously
  crafted update for a response policy zone (RPZ). This has been fixed.
  :gl:`#5775` :gl:`!11687`

- Fix use-after-free in xfrin_recv_done. ``46099d2d9af``

  Move the LIBDNS_XFRIN_RECV_DONE probe execution before
  dns_xfrin_detach in xfrin_recv_done.

  Previously, dns_xfrin_detach was called before the trace probe, which
  could free the xfr object.  Because the accessed member xfr->info is
  an embedded array, the expression evaluates via pointer arithmetic
  rather than a direct memory dereference.  Although this prevents a
  reliable crash in practice, it technically remains a use-after-free
  issue. Reorder the statements to ensure the transfer context is fully
  valid when the probe executes. :gl:`#5786` :gl:`!11694`

- Backport test for update-policy per-type max quota bypass via crafted
  UPDATE messages. ``545ce3ae224``

  An authenticated DDNS client could bypass update-policy per-type
  record limits (e.g. TXT(3)) by including padding records in the UPDATE
  message that are silently skipped during processing in the main
  branch.

  As BIND 9.20 is not affected, only backport the test. :gl:`#5799`
  :gl:`!11760`

- Fix a crash triggered by rndc modzone on zone from configuration file.
  ``6d9482bd6b8``

  Calling `rndc modzone` on a zone that was configured in the
  configuration file caused a crash. This has been fixed.

  ISC would like to thank Nathan Reilly for reporting this. :gl:`#5800`
  :gl:`!11698`

- Fix the processing of empty catalog zone ACLs. ``ce365083d9d``

  The :iscman:`named` process could terminate unexpectedly when
  processing a catalog zone ACL in an APL resource record that was
  completely empty. This has been fixed. :gl:`#5801` :gl:`!11759`

- Fix OpenSSL 4 compatibility issue when calling X509_get_subject_name()
  ``1d43bf8263f``

  Starting from OpenSSL 4 the the X509_get_subject_name() function
  returns a 'const' pointer to a name instead of a regular pointer.
  Duplicate the name before operating on it, then free it. :gl:`#5807`
  :gl:`!11692`

- Take dns_dtenv_t reference before an async function call.
  ``be7b811fffc``

  A 'dns_dtenv_t' pointer is passed to an async function without taking
  a reference first, which can potentially cause a use-after-free error.
  Take a reference, then detach in the async function. :gl:`#5820`
  :gl:`!11714`

- Fix a crash triggered by rndc modzone on zone that already existed in
  NZF file. ``46dbcd7c9a5``

  Calling `rndc modzone` didn't work properly for a zone hat was
  configured in  the configuration file. It could crash if BIND 9 was
  built without LMDB or if  there was already an NZF file for the zone.
  In addition, `rndc modzone` failed in subsequent attempts. These
  problems are now fixed. :gl:`#5826` :gl:`!11743`

- Fix couple of reference counting bugs. ``fc5e26cfc9f``

  Fix missing detach/free on error paths. :gl:`!11691`

- Fix data race in server round-trip time tracking. ``31cbfc9fb36``

  The SRTT (Smoothed Round-Trip Time) update for remote servers was not
  atomic — concurrent callers could each read the same value and one
  update would be silently lost. Additionally, the aging decay applied
  once per second could run multiple times if several threads entered
  the function simultaneously.

  Use compare-and-swap loops for the SRTT update and for the aging
  timestamp to ensure no updates are lost. :gl:`!11723`

- Fix data race on fctx->vresult in validated() ``996c66aee7a``

  Move the write to fctx->vresult after LOCK(&fctx->lock).  The field
  was being set before acquiring the lock, but dns_resolver_logfetch()
  reads it under the same lock from another thread. :gl:`!11721`

- Fix isc_buffer_init capacity mismatch in DoH data chunk callback.
  ``f0a2b07359c``

  isc_buffer_init() is given MAX_DNS_MESSAGE_SIZE (65535) as capacity
  but only h2->content_length bytes are allocated.  This makes the
  buffer believe it has more space than actually allocated.  A secondary
  bounds check (new_bufsize <= h2->content_length) prevents actual
  overflow, but the buffer invariant is violated.

  Pass h2->content_length as the capacity to match the allocation.
  :gl:`!11709`

- Fix memory leak in dns_catz_options_setdefault() for zonedir.
  ``1844afec7ba``

  When defaults->zonedir is set, opts->zonedir is unconditionally
  overwritten without freeing the previous value. This leaks memory on
  every catalog zone update when zonedir defaults are configured.

  Free the existing opts->zonedir before replacing it. :gl:`!11685`

- Fix potential resource during resolver error handling. ``6a32c1acdc7``

  Under specific error conditions during query processing, resources
  were not being properly released, which could eventually lead to
  unnecessary memory consumption for the server.  The a potential
  resource leak in the resolver has been fixed. :gl:`!11706`

- Fix resquery reference imbalance on TCP connect failure.
  ``b69bbf2e0ed``

  In fctx_query(), resquery_ref(query) is called before
  dns_dispatch_connect() in anticipation of the resquery_connected()
  callback consuming the reference.  When dns_dispatch_connect() fails
  synchronously on TCP (e.g. from dns_transport_get_tlsctx() failing in
  tcp_dispatch_connect()), the connect callback is never scheduled, so
  the extra reference is never consumed.  This has been fixed.
  :gl:`!11656`


