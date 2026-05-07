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

BIND 9.20.17
------------

New Features
~~~~~~~~~~~~

- Add spatch to detect implicit bool/int/result cast. ``02be363d1f``

  Detection of implicit cast from a boolean into an int, or an
  isc_result_t into a boolean (either in an assignement or return
  position).

  If such pattern is found, a warning comment is added into the code
  (and the CI will fails) so the error can be spotted and manually
  fixed. :gl:`!11237`

Feature Changes
~~~~~~~~~~~~~~~

- Use atomics for CMM_{LOAD,STORE}_SHARED with ThreadSanitizer.
  ``94fa721705``

  Upstream has removed the atomics implementation of CMM_LOAD_SHARED and
  CMM_STORE_SHARED as these can be used also with non-stdatomics types.
  As we only use the CMM api with stdatomics types, we can restore the
  previous behaviour to prevent ThreadSanitizer warnings. :gl:`#5660`
  :gl:`!11290`

- Provide more information when the memory allocation fails.
  ``6749725610``

  Provide more information about the failure when the memory allocation
  fails. :gl:`!11304`

- Reduce the number of outgoing queries. ``457b470e96``

  Reduces the number of outgoing queries when resolving the nameservers
  for delegation points.  This helps the DNS resolver with cold cache
  resolve client queries with complex delegation chains and
  redirections. :gl:`!11258`

Bug Fixes
~~~~~~~~~

- Fix the spurious timeouts while resolving names. ``d96cf874fb``

  Sometimes the loops in the resolving (e.g. to resolve or validate
  ns1.example.com we need to resolve ns1.example.com) were not properly
  detected leading to spurious 10 seconds delay.  This has been fixed
  and such loops are properly detected. :gl:`#3033`, #5578 :gl:`!11298`

- Fix bug where zone switches from NSEC3 to NSEC after retransfer.
  ``3b40ffbf83``

  When a zone is re-transferred, but the zone journal on an
  inline-signing secondary is out of sync, the zone could fall back to
  using NSEC records instead of NSEC3. This has been fixed. :gl:`#5527`
  :gl:`!11274`

- Attach socket before async streamdns_resume_processing. ``bb9451c73f``

  Call to `streamdns_resume_processing` is asynchronous but the socket
  passed as argument is not attached when scheduling the call.

  While there is no reproducible way (so far) to make the socket
  reference number down to 0 before `streamdns_resume_processing` is
  called, attach the socket before scheduling the call. This guard
  against an hypothetic case where, for some reasons, the socket
  refcount would reach 0, and be freed from memory when
  `streamdns_resume_processing` is called. :gl:`#5620` :gl:`!11260`

- AMTRELAY type 0 presentation format handling was wrong. ``adf104a063``

  RFC 8777 specifies a placeholder value of "." for the gateway field
  when the gateway type is 0 (no gateway).  This was not being checked
  for nor emitted when displaying the record. This has been corrected.

  Instances of this record will need the placeholder period added to
  them when upgrading. :gl:`#5639` :gl:`!11255`

- Fix parsing bug in remote-servers with key or tls. ``d9400c5967``

  The :any:`remote-servers` clause enable the following pattern using a
  named ``server-list``:

  remote-servers a { 1.2.3.4; ... };         remote-servers b { a key
  foo; };

  However, such configuration was wrongly rejected, with an "unexpected
  token 'foo'" error. Such configuration is now accepted. :gl:`#5646`
  :gl:`!11300`

- Fix TLS contexts cache object usage bug in the resolver.
  ``13adf94006``

  :iscman:`named` could terminate unexpectedly when reconfiguring or
  reloading, and if client-side TLS transport was in use (for example,
  when forwarding queries to a DoT server). This has been fixed.
  :gl:`#5653` :gl:`!11299`

- Fix unitiailized pointer check on getipandkeylist. ``5ed0cf091b``

  Function `named_config_getipandkeylist` could, in case of error in the
  early code attempting to get the `port` or `tls-port`, make a pointer
  check on a non-initialized value. This is now fixed. :gl:`!11306`

- Standardize CHECK and RETERR macros. ``ef714e91ac``

  previously, there were over 40 separate definitions of CHECK macros,
  of which most used "goto cleanup", and the rest "goto failure" or
  "goto out". there were another 10 definitions of RETERR, of which most
  were identical to CHECK, but some simply returned a result code
  instead of jumping to a cleanup label.

  this has now been standardized throughout the code base: RETERR is for
  returning an error code in the case of an error, and CHECK is for
  jumping to a cleanup tag, which is now always called "cleanup". both
  macros are defined in isc/util.h. :gl:`!11069`

- Adding NSEC3 opt-out records could leave invalid records in
  chain. ``1d83a8ad46``

  When creating an NSEC3 opt-out chain, a node in the chain could be
  removed too soon, causing the previous NSEC3 being unable to be found,
  resulting in invalid NSEC3 records to be left in the zone. This has
  been fixed.

  Closes [#5671](#5671)

