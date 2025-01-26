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

BIND 9.20.4
-----------

New Features
~~~~~~~~~~~~

- Update bind.keys with the new 2025 IANA root key. ``1f988e2cc7``

  Add an 'initial-ds' entry to bind.keys for the new root key, ID 38696,
  which is scheduled for publication in January 2025. :gl:`#4896`
  :gl:`!9746`

- Support jinja2 templates in pytest runner. ``4a9380835f``

  Configuration files in system tests which require some variables (e.g.
  port numbers) filled in during test setup, can now use jinja2
  templates when `jinja2` python package is available.

  Any `*.j2` file found within the system test directory will be
  automatically rendered with the environment variables into a file
  without the `.j2` extension by the pytest runner. E.g.
  `ns1/named.conf.j2` will become `ns1/named.conf` during test setup. To
  avoid automatic rendering, use `.j2.manual` extension and render the
  files manually at test time.

  New `templates` pytest fixture has been added. Its `render()` function
  can be used to render a template with custom test variables. This can
  be useful to fill in different config options during the test. With
  advanced jinja2 template syntax, it can also be used to include/omit
  entire sections of the config file rather than using `named1.conf.in`,
  `named2.conf.in` etc. :gl:`#4938` :gl:`!9699`

Removed Features
~~~~~~~~~~~~~~~~

- Move contributed DLZ modules into a separate repository.
  ``a1cd30cd25``

  The DLZ modules are poorly maintained as we only ensure they can still
  be compiled, the DLZ interface is blocking, so anything that blocks
  the query to the database blocks the whole server and they should not
  be used except in testing.  The DLZ interface itself is going to be
  scheduled for removal.

  The DLZ modules now live in
  https://gitlab.isc.org/isc-projects/dlz-modules repository.
  :gl:`#4865` :gl:`!9777`

Feature Changes
~~~~~~~~~~~~~~~

- Use lists of expected artifacts in system tests. ``e5fa109599``

  ``clean.sh`` scripts have been replaced by lists of expected artifacts
  for each system test module. The list is defined using the custom
  ``pytest.mark.extra_artifacts`` mark, which can use both filenames and
  globs. :gl:`#4261` :gl:`!9734`

- Dnssec-ksr now supports KSK rollovers. ``834c04fc77``

  The tool 'dnssec-ksr' now allows for KSK generation, as well as
  planned KSK rollovers. When signing a bundle from a Key Signing
  Request (KSR), only the key that is active in that time frame is being
  used for signing. Also, the CDS and CDNSKEY records are now added and
  removed at the correct time. :gl:`#4697`  :gl:`#4705` :gl:`!9711`

- Incrementally apply AXFR transfer. ``4509b92e21``

  Reintroduce logic to apply diffs when the number of pending tuples is
  above 128. The previous strategy of accumulating all the tuples and
  pushing them at the end leads to excessive memory consumption during
  transfer.

  This effectively reverts half of e3892805d6 :gl:`#4986` :gl:`!9761`

- Print expire option in transfer summary. ``6cd001a68b``

  The zone transfer summary will now print the expire option value in
  the zone transfer summary. :gl:`#5013` :gl:`!9714`

- Add two new clang-format options that help with code formatting.
  ``4230b2b514``

  * Add new clang-format option to remove redundant semicolons
  * Add new clang-format option to remove redundant parentheses

  :gl:`!9750`

- Emit more helpful log for exceeding max-records-per-type.
  ``74e7e229f2``

  The new log message is emitted when adding or updating an RRset fails
  due to exceeding the max-records-per-type limit. The log includes the
  owner name and type, corresponding zone name, and the limit value. It
  will be emitted on loading a zone file, inbound zone transfer (both
  AXFR and IXFR), handling a DDNS update, or updating a cache DB. It's
  especially helpful in the case of zone transfer, since the secondary
  side doesn't have direct access to the offending zone data.

  It could also be used for max-types-per-name, but this change doesn't
  implement it yet as it's much less likely to happen in practice.
  :gl:`!9771`

- Harden key management when key files have become unavailabe.
  ``11b0f41f80``

  Prior to doing key management, BIND 9 will check if the key files on
  disk match the expected keys. If key files for previously observed
  keys have become unavailable, this will prevent the internal key
  manager from running. :gl:`!9622`

Bug Fixes
~~~~~~~~~

- Use TLS for notifies if configured to do so. ``c1b82c1fb8``

  Notifies configured to use TLS will now be sent over TLS, instead of
  plaintext UDP or TCP. Also, failing to load the TLS configuration for
  notify now also results in an error. :gl:`#4821` :gl:`!9684`

- '{&dns}' is as valid as '{?dns}' in a SVCB's dohpath. ``b27cb14616``

  `dig` fails to parse a valid (as far as I can tell, and accepted by
  `kdig` and `Wireshark`) `SVCB` record with a `dohpath` URI template
  containing a `{&dns}`, like `dohpath=/some/path?key=value{&dns}"`. If
  the URI template contains a `{?dns}` instead `dig` is happy, but my
  understanding of rfc9461 and section 1.2. "Levels and Expression
  Types" of rfc6570 is that `{&dns}` is valid. See for example section
  1.2. "Levels and Expression Types" of rfc6570.

  Note that Peter van Dijk suggested that `{dns}` and
  `{dns,someothervar}` might be valid forms as well, so my patch might
  be too restrictive, although it's anyone's guess how DoH clients would
  handle complex templates. :gl:`#4922` :gl:`!9769`

- Make dns_validator_cancel() respect the data ownership. ``8002fda38c``

  There was a data race dns_validator_cancel() was called when the
  offloaded operations were in progress.  Make dns_validator_cancel()
  respect the data ownership and only set new .canceling variable when
  the offloaded operations are in progress.  The cancel operation would
  then finish when the offloaded work passes the ownership back to the
  respective thread. :gl:`#4926` :gl:`!9790`

- Fix NSEC3 closest encloser lookup for names with empty non-terminals.
  ``76dc8accd3``

  The performance improvement for finding the NSEC3 closest encloser
  when generating authoritative responses could cause servers to return
  incorrect NSEC3 records in some cases. This has been fixed.
  :gl:`#4950` :gl:`!9631`

- Revert "Improve performance when looking for the closest encloser"
  ``29c460a4e5``

  Revert "fix: chg: Improve performance when looking for the closest
  encloser when returning NSEC3 proofs"

  This reverts merge request !9436 :gl:`#4950` :gl:`!9613`

- Fix a data race in dns_zone_getxfrintime() ``dd72a5eb8d``

  The dns_zone_getxfrintime() function fails to lock the zone before
  accessing its 'xfrintime' structure member, which can cause a data
  race between soa_query() and the statistics channel. Add the missing
  locking/unlocking pair, like it's done in numerous other similar
  functions. :gl:`#4976` :gl:`!9601`

- 'Recursive-clients 0;' triggers an assertion. ``747a19bc00``

  BIND 9.20.0 broke `recursive-clients 0;`.  This has now been fixed.
  :gl:`#4987` :gl:`!9654`

- Transport needs to be a selector when looking for an existing
  dispatch. ``09fb8e354a``

  This allows for dispatch to use existing TCP/HTTPS/TLS etc. streams
  without accidentally using an unexpected transport. :gl:`#4989`
  :gl:`!9682`

- Parsing of hostnames in rndc.conf was broken. ``b46f2376d0``

  When DSCP support was removed, parsing of hostnames in rndc.conf was
  accidentally broken, resulting in an assertion failure.  This has been
  fixed. :gl:`#4991` :gl:`!9676`

- Restore values when dig prints command line. ``f604c31ad2``

  Options of the form `[+-]option=<value>` failed to display the value
  on the printed command line. This has been fixed. :gl:`#4993`
  :gl:`!9666`

- Provide more visibility into configuration errors. ``41fd5e9955``

  by logging SSL_CTX_use_certificate_chain_file and
  SSL_CTX_use_PrivateKey_file errors individually. :gl:`#5008`
  :gl:`!9767`

- Fix a data race between dns_zone_getxfr() and dns_xfrin_create()
  ``2cb91e0631``

  There is a data race between the statistics channel, which uses
  `dns_zone_getxfr()` to get a reference to `zone->xfr`, and the
  creation of `zone->xfr`, because the latter happens outside of a zone
  lock.

  Split the `dns_xfrin_create()` function into two parts to separate the
  zone transfer starting part from the zone transfer object creation
  part. This allows us to attach the new object to a local variable
  first, then attach it to `zone->xfr` under a lock, and only then start
  the transfer. :gl:`#5011` :gl:`!9728`

- Fix race condition when canceling ADB find. ``668ea24467``

  When canceling the ADB find, the lock on the find gets released for a
  brief period of time to be locked again inside adbname lock.  During
  the brief period that the ADB find is unlocked, it can get canceled by
  other means removing it from the adbname list which in turn causes
  assertion failure due to a double removal from the adbname list. This
  has been fixed. :gl:`#5024` :gl:`!9744`

- Improve the memory cleaning in the SERVFAIL cache. ``fa5d270f95``

  The SERVFAIL cache doesn't have a memory bound and the cleaning of the
  old SERVFAIL cache entries was implemented only in opportunistic
  manner.  Improve the memory cleaning of the SERVFAIL cache to be more
  aggressive, so it doesn't consume a lot of memory in the case the
  server encounters many SERVFAILs at once. :gl:`#5025` :gl:`!9794`

- Fix trying the next primary server when the preivous one was marked as
  unreachable. ``ab138bb717``

  In some cases (there is evidence only when XoT was used) `named`
  failed to try the next primary server in the list when the previous
  one was marked as unreachable. This has been fixed. :gl:`#5038`
  :gl:`!9788`

- Clean up 'nodetach' in ns_client. ``47a77a3b12``

  The 'nodetach' member is a leftover from the times when non-zero
  'stale-answer-client-timeout' values were supported, and currently is
  always 'false'. Clean up the member and its usage. :gl:`!9600`

- Fix error path bugs in the manager's "recursing-clients" list
  management. ``d2ea42e237``

  In two places, after linking the client to the manager's
  "recursing-clients" list using the check_recursionquota() function,
  the query.c module fails to unlink it on error paths. Fix the bugs by
  unlinking the client from the list. :gl:`!9604`

- Remove unused <openssl/{hmac,engine}.h> headers from OpenSSL shims.
  ``6d717e88c0``

  The <openssl/{hmac,engine}.h> headers were unused and including the
  <openssl/engine.h> header might cause build failure when OpenSSL
  doesn't have Engines support enabled.

  See https://fedoraproject.org/wiki/Changes/OpensslDeprecateEngine
  :gl:`!9593`

- Use attach()/detach() functions instead of touching .references.
  ``1e9c3af75a``

  In rbtdb.c, there were places where the code touched .references
  directly instead of using the helper functions.  Use the helper
  functions instead.

  Forward port from
  https://gitlab.isc.org/isc-private/bind9/-/merge_requests/753
  :gl:`!9795`


