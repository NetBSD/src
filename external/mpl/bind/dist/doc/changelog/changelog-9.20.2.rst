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

BIND 9.20.2
-----------

New Features
~~~~~~~~~~~~

- Support for Offline KSK implemented. ``3555094a686``

  Add a new configuration option `offline-ksk` to enable Offline KSK key
  management. Signed Key Response (SKR) files created with `dnssec-ksr`
  (or other program) can now be imported into `named` with the new `rndc
  skr -import` command. Rather than creating new DNSKEY, CDS and CDNSKEY
  records and generating signatures covering these types, these records
  are loaded from the currently active bundle from the imported SKR.

  The implementation is loosely based on:
  https://web.archive.org/web/20250121040252/https://www.iana.org/dnssec/archive/files/draft-icann-dnssec-keymgmt-01.txt
  :gl:`#1128` :gl:`!9119`

- Print the full path of the working directory in startup log messages.
  ``1c8eeafffb0``

  named now prints its initial working directory during startup and the
  changed working directory when loading or reloading its configuration
  file if it has a valid 'directory' option defined. :gl:`#4731`
  :gl:`!9372`

- Support restricted key tag range when generating new keys.
  ``d0899632635``

  It is useful when multiple signers are being used to sign a zone to
  able to specify a restricted range of range of key tags that will be
  used by an operator to sign the zone.  This adds controls to named
  (dnssec-policy), dnssec-signzone, dnssec-keyfromlabel and dnssec-ksr
  (dnssec-policy) to specify such ranges. :gl:`#4830` :gl:`!9396`

Feature Changes
~~~~~~~~~~~~~~~

- Exempt prefetches from the fetches-per-zone and fetches-per-server
  quotas. ``5e78cade523``

  Fetches generated automatically as a result of 'prefetch' are now
  exempt from the 'fetches-per-zone' and 'fetches-per-server' quotas.
  This should help in maintaining the cache from which query responses
  can be given. :gl:`#4219` :gl:`!9420`

- Restore the ability to select individual unit tests. ``cfac05cc966``

  This adds the command line arguments: `-d` (debug), `-l` (list tests)
  and `-t test` (run this test) to the unit tests, e.g.:

  .. code::

     % ./rdata_test -t zonemd
     [==========] selected:
     Running 1 test(s).
     [ RUN      ] zonemd
     [       OK ] zonemd
     [==========] selected: 1 test(s) run.
     [  PASSED  ] 1 test(s).
     %

  :gl:`#4579` :gl:`!9385`

- Process also the ISC_R_CANCELED result code in rpz_rewrite()
  ``eb2e0991e1a``

  Log canceled resolver queries (e.g. when shutting down a hung fetch)
  in DEBUG3 level instead of DEBUG1 which is used for the "unrecognized"
  result codes. :gl:`#4797` :gl:`!9347`

- Remove code to read and parse /proc/net/if_inet6 on Linux.
  ``e3cc5034ab0``

  The getifaddr() works fine for years, so we don't have to keep the
  callback to parse /proc/net/if_inet6 anymore. :gl:`#4852` :gl:`!9341`

- Use seteuid()/setegid() instead of setreseuid()/setresgid()
  ``1127b2b3d16``

  It looks like that all supported systems now have support for
  _POSIX_SAVED_IDS, so it's safe to use setegid() and setegid() because
  those will not change saved used/group IDs. :gl:`#4862` :gl:`!9371`

- Follow the number of CPU set by taskset/cpuset. ``ce3209b1dcf``

  Administrators may wish to constrain the set of cores that BIND 9 runs
  on via the 'taskset', 'cpuset' or 'numactl' programs (or equivalent on
  other O/S).

  If the admin has used taskset, the `named` will now follow to
  automatically use the given number of CPUs rather than the system wide
  count. :gl:`#4884` :gl:`!9442`

- Double the number of threadpool threads. ``cfdded46676``

  Introduce this temporary workaround to reduce the impact of long-
  running tasks in offload threads which can block the resolution of
  queries. :gl:`#4898`

Bug Fixes
~~~~~~~~~

- Delay release of root privileges until after configuring controls.
  ``0b7eb9d7a90``

  Delay relinquishing root privileges until the control channel has been
  configured, for the benefit of systems that require root to use
  privileged port numbers.  This mostly affects systems without fine-
  grained privilege systems (i.e., other than Linux). :gl:`#4793`
  :gl:`!9444`

- Fix the assertion failure in the isc_hashmap iterator. ``92e54fa9b7f``

  When the round robin hashing reorders the map entries on deletion, we
  were adjusting the iterator table size only when the reordering was
  happening at the internal table boundary.  The iterator table size had
  to be reduced by one to prevent seeing the entry that resized on
  position [0] twice because it migrated to [iter->size - 1] position.

  However, the same thing could happen when the same entry migrates a
  second time from [iter->size - 1] to [iter->size - 2] position (and so
  on) because the check that we are manipulating the entry just in the
  [0] position was insufficient.  Instead of checking the position [pos
  == 0], we now check that the [pos % iter->size == 0], thus ignoring
  all the entries that might have moved back to the end of the internal
  table. :gl:`#4838` :gl:`!9310`

- Add -Wno-psabi to CFLAGS for x86 (32-bit) builds. ``9f2061e31eb``

  GCC 11.1+ emits a note during compilation when there are 64-bit atomic
  fields in a structure, because it fixed a compiler bug by changing the
  alignment of such fields, which caused ABI change.

  Add -Wno-psabi to CFLAGS for such builds in order to silence the
  warning. That shouldn't be a problem since we don't expose our
  structures to the outside. :gl:`#4841` :gl:`!9322`

- Check if logconfig is NULL before using it in isc_log_doit()
  ``11cb3767256``

  Check if 'lctx->logconfig' is NULL before using it in isc_log_doit(),
  because it's possible that isc_log_destroy() was already called, e.g.
  when a 'call_rcu' function wants to log a message during shutdown.
  :gl:`#4842` :gl:`!9323`

- Change the NS_PER_SEC (and friends) from enum to static const.
  ``91cecebf4c6``

  New version of clang (19) has introduced a stricter checks when mixing
  integer (and float types) with enums.  In this case, we used enum {}
  as C17 doesn't have constexpr yet.  Change the time conversion
  constants to be static const unsigned int instead of enum values.
  :gl:`#4845` :gl:`!9339`

- Check the result of dirfd() before calling unlinkat() ``335796f32a1``

  Instead of directly using the result of dirfd() in the unlinkat()
  call, check whether the returned file descriptor is actually valid.
  That doesn't really change the logic as the unlinkat() would fail with
  invalid descriptor anyway, but this is cleaner and will report the
  right error returned directly by dirfd() instead of EBADF from
  unlinkat(). :gl:`#4853` :gl:`!9343`

- Fix rare assertion failure when shutting down incoming transfer.
  ``02d4755cc31``

  A very rare assertion failure can be triggered when the incoming
  transfer is either forcefully shut down or it is finished during
  printing the details about the statistics channel.  This has been
  fixed. :gl:`#4860` :gl:`!9377`

- Fix the resesuid() shim implementation for NetBSD. ``d959c035e89``

  The shim implementation of setresuid() was wrong - there was a copy
  and paste error and it was calling setresgid() instead.  This only
  affects NetBSD because Linux, FreeBSD and OpenBSD have setresuid() and
  setresgid() implementation available from the system library.
  :gl:`#4862` :gl:`!9361`

- Fix algoritm rollover bug when there are two keys with the same
  keytag. ``2f2003c55d4``

  If there is an algorithm rollover and two keys of different algorithm
  share the same keytags, then there is a possibility that if we check
  that a key matches a specific state, we are checking against the wrong
  key. This has been fixed by not only checking for matching key tag but
  also key algorithm. :gl:`#4878` :gl:`!9393`

- Stop using malloc_usable_size and malloc_size. ``1b7fa52d8ff``

  The `malloc_usable_size()` can return size larger than originally
  allocated and when these sizes disagree the fortifier enabled by
  `_FORTIFY_SOURCE=3` detects overflow and stops the `named` execution
  abruptly.  Stop using these convenience functions as they are primary
  used for introspection-only. :gl:`#4880` :gl:`!9418`

- Preserve statement tag order in documentation. ``57a9e3da00c``

  This supports bit-for-bit reproducibility of built documentation.
  :gl:`#4886` :gl:`!9408`

- Fix an assertion failure in validate_dnskey_dsset_done()
  ``870f0be27eb``

  Under rare circumstances, named could terminate unexpectedly when
  validating a DNSKEY resource record if the validation was canceled in
  the meantime. This has been fixed. :gl:`#4911`

- Silence all warnings that stem from the default config.
  ``dde38470476``

  As we now setup the logging very early, parsing the default config
  would always print warnings about experimental (and possibly
  deprecated) options in the default config.  This would even mess with
  commands like `named -V` and it is also wrong to warn users about
  using experimental options in the default config, because they can't
  do anything about this.  Add CFG_PCTX_NODEPRECATED and
  CFG_PCTX_NOEXPERIMENTAL options that we can pass to cfg parser and
  silence the early warnings caused by using experimental options in the
  default config. :gl:`!9305`

