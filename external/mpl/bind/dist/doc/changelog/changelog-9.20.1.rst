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

BIND 9.20.1
-----------

New Features
~~~~~~~~~~~~

- Tighten 'max-recursion-queries' and add 'max-query-restarts' option.
  ``42e70b0f0e``

  There were cases in resolver.c when the `max-recursion-queries` quota
  was ineffective. It was possible to craft zones that would cause a
  resolver to waste resources by sending excessive queries while
  attempting to resolve a name. This has been addressed by correcting
  errors in the implementation of `max-recursion-queries`, and by
  reducing the default value from 100 to 32.

  In addition, a new `max-query-restarts` option has been added which
  limits the number of times a recursive server will follow CNAME or
  DNAME records before terminating resolution. This was previously a
  hard-coded limit of 16, and now defaults to 11.   :gl:`#4741`
  :gl:`!9282`

- Implement rndc retransfer -force. ``008bfb6249``

  A new optional argument '-force' has been added to the command channel
  command 'rndc retransfer'. When it is specified, named aborts the
  ongoing zone transfer (if there is one), and starts a new transfer.
  :gl:`#2299` :gl:`!9219`

- Generate changelog from git log. ``cf60eb2738``

  Use a single source of truth, the git log, to generate the list of
  CHANGES. Use the .rst format and include it in the ARM for a quick
  reference with proper gitlab links to issues and merge requests.
  :gl:`#75` :gl:`!9180`

Feature Changes
~~~~~~~~~~~~~~~

- Call rcu_barrier() in the isc_mem_destroy() just once. ``e00b13ac6e``

  The previous work in this area was led by the belief that we might be
  calling call_rcu() from within call_rcu() callbacks.  After carefully
  checking all the current callback, it became evident that this is not
  the case and the problem isn't enough rcu_barrier() calls, but
  something entirely else.

  Call the rcu_barrier() just once as that's enough and the multiple
  rcu_barrier() calls will not hide the real problem anymore, so we can
  find it. :gl:`!9247`

- Don't open route socket if we don't need it. ``4f369af51e``

  When automatic-interface-scan is disabled, the route socket was still
  being opened. Add new API to connect / disconnect from the route
  socket only as needed.

  Additionally, move the block that disables periodic interface rescans
  to a place where it actually have access to the configuration values.
  Previously, the values were being checked before the configuration was
  loaded. :gl:`!9239`

- Allow shorter resolver-query-timeout configuration. ``840e56a979``

  The minimum allowed value of 'resolver-query-timeout' was lowered to
  301 milliseconds instead of the earlier 10000 milliseconds (which is
  the default). As earlier, values less than or equal to 300 are
  converted to seconds before applying the limit. :gl:`#4320`
  :gl:`!9220`

- Replace `#define DNS_GETDB_` with struct of bools. ``6d1fdb8505``

  Replace `#define DNS_GETDB_` with struct of bools to make it easier to
  pretty-print the attributes in a debugger. :gl:`#4559` :gl:`!9205`

- Fix data race in clean_finds_at_name. ``be1e649974``

  Stop updating `find.result_v4` and `find.result_v4` in
  `clean_finds_at_name`. The values are supposed to be
  static. :gl:`#4118` :gl:`!9197`

Bug Fixes
~~~~~~~~~

- Reconfigure catz member zones during named reconfiguration.
  ``9a0c59c89a``

  During a reconfiguration named wasn't reconfiguring catalog zones'
  member zones. This has been fixed. :gl:`#4733`

- Disassociate the SSL object from the cached SSL_SESSION.
  ``54b24fb015``

  When the SSL object was destroyed, it would invalidate all SSL_SESSION
  objects including the cached, but not yet used, TLS session objects.

  Properly disassociate the SSL object from the SSL_SESSION before we
  store it in the TLS session cache, so we can later destroy it without
  invalidating the cached TLS sessions. :gl:`#4834` :gl:`!9274`

- Attach/detach to the listening child socket when accepting TLS.
  ``24ac7a7cd2``

  When TLS connection (TLSstream) connection was accepted, the children
  listening socket was not attached to sock->server and thus it could
  have been freed before all the accepted connections were actually
  closed.

  In turn, this would cause us to call isc_tls_free() too soon - causing
  cascade errors in pending SSL_read_ex() in the accepted connections.

  Properly attach and detach the children listening socket when
  accepting and closing the server connections. :gl:`#4833` :gl:`!9273`

- Fix --enable-tracing build on systems without dtrace. ``d8d49c9340``

  Missing file util/dtrace.sh prevented builds on system without dtrace
  utility. This has been corrected.

- Make hypothesis optional for system tests. ``c5f1cb8a04``

  Ensure that system tests can be executed without Python hypothesis
  package. :gl:`#4831` :gl:`!9267`

- Dig now reports missing query section for opcode QUERY. ``b277a6f1f0``

  Query responses should contain the question section with some
  exceptions.  Dig was not reporting this. :gl:`#4808` :gl:`!9269`

- Fix assertion failure in the glue cache. ``f8a0c0bed6``

  Fix an assertion failure that could happen as a result of data race
  between free_gluetable() and addglue() on the same headers.
  :gl:`#4691` :gl:`!9256`

- Don't use 'create' flag unnecessarily in findnode() ``4281aaab45``

  when searching the cache for a node so that we can delete an rdataset,
  it isn't necessary to set the 'create' flag. if the node doesn't exist
  yet, we won't be able to delete anything from it anyway. :gl:`!9253`

- Raise the log level of priming failures. ``074c7cc12c``

  When a priming query is complete, it's currently logged at level
  ISC_LOG_DEBUG(1), regardless of success or failure. We are now raising
  it to ISC_LOG_NOTICE in the case of failure. [GL #3516] :gl:`#3516`
  :gl:`!9250`

- Fix assertion failure when checking named-checkconf version.
  ``42e84e4b97``

  Checking the version of `named-checkconf` would end with assertion
  failure.  This has been fixed. :gl:`#4827` :gl:`!9246`

- Valid TSIG signatures with invalid time cause crash. ``2438db2eae``

  An assertion failure triggers when the TSIG has valid cryptographic
  signature, but the time is invalid. This can happen when the times
  between the primary and secondary servers are not synchronised.
  :gl:`#4811` :gl:`!9245`

- Don't skip the counting if fcount_incr() is called with force==true.
  ``9cd2880a82``

  The fcount_incr() was incorrectly skipping the accounting for the
  fetches-per-zone if the force argument was set to true. We want to
  skip the accounting only when the fetches-per-zone is completely
  disabled, but for individual names we need to do the accounting even
  if we are forcing the result to be success. :gl:`#4786` :gl:`!9241`

- Don't skip the counting if fcount_incr() is called with force==true
  (v2) ``1db5c6a0d3``

  The fcount_incr() was not increasing counter->count when force was set
  to true, but fcount_decr() would try to decrease the counter leading
  to underflow and assertion failure.  Swap the order of the arguments
  in the condition, so the !force is evaluated after incrementing the
  .count. :gl:`#4846` :gl:`!9299`


- Fix PTHREAD_MUTEX_ADAPTIVE_NP and PTHREAD_MUTEX_ERRORCHECK_NP usage.
  ``46caf5f4a4``

  The PTHREAD_MUTEX_ADAPTIVE_NP and PTHREAD_MUTEX_ERRORCHECK_NP are
  usually not defines, but enum values, so simple preprocessor check
  doesn't work.

  Check for PTHREAD_MUTEX_ADAPTIVE_NP from the autoconf
  AS_COMPILE_IFELSE block and define HAVE_PTHREAD_MUTEX_ADAPTIVE_NP.
  This should enable adaptive mutex on Linux and FreeBSD.

  As PTHREAD_MUTEX_ERRORCHECK actually comes from POSIX and Linux glibc
  does define it when compatibility macros are being set, we can just
  use PTHREAD_MUTEX_ERRORCHECK instead of PTHREAD_MUTEX_ERRORCHECK_NP.
  :gl:`!9240`

- Remove extra newline from yaml output. ``53738634c3``

  I split this into two commits, one for the actual newline removal, and
  one for issues I found, ruining the yaml output when some errors were
  outputted.

- CID 498025 and CID 498031: Overflowed constant INTEGER_OVERFLOW.
  ``b6298b394e``

  Add INSIST to fail if the multiplication would cause the variables to
  overflow. :gl:`#4798` :gl:`!9229`

- Remove unnecessary operations. ``067f87f158``

  Decrementing optlen immediately before calling continue is unneccesary
  and inconsistent with the rest of dns_message_pseudosectiontoyaml and
  dns_message_pseudosectiontotext.  Coverity was also reporting an
  impossible false positive overflow of optlen (CID 499061). :gl:`!9223`

- Fix generation of 6to4-self name expansion from IPv4 address.
  ``00ce93a69c``

  The period between the most significant nibble of the encoded IPv4
  address and the 2.0.0.2.IP6.ARPA suffix was missing resulting in the
  wrong name being checked. Add system test for 6to4-self
  implementation. :gl:`#4766` :gl:`!9217`

- Fix false QNAME minimisation error being reported. ``fb07c38697``

  Remove the false positive "success resolving" log message when QNAME
  minimisation is in effect and the final result is NXDOMAIN.
  :gl:`#4784` :gl:`!9215`

- Dig +yaml was producing unexpected and/or invalid YAML output.
  ``a42afbce2e``

  :gl:`#4796` :gl:`!9213`

- SVBC alpn text parsing failed to reject zero length alpn.
  ``1a1413ff59``

  :gl:`#4775` :gl:`!9209`

- Return SERVFAIL for a too long CNAME chain. ``d7e5f7903d``

  When cutting a long CNAME chain, named was returning NOERROR  instead
  of SERVFAIL (alongside with a partial answer). This has been fixed.
  :gl:`#4449` :gl:`!9203`

- Properly calculate the amount of system memory. ``c63b7fad49``

  On 32 bit machines isc_meminfo_totalphys could return an incorrect
  value. :gl:`#4799` :gl:`!9199`

- Update key lifetime and metadata after dnssec-policy reconfig.
  ``a5f554959e``

  Adjust key state and timing metadata if dnssec-policy key lifetime
  configuration is updated, so that it also affects existing keys.
  :gl:`#4677` :gl:`!9191`

