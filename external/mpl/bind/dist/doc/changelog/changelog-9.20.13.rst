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

BIND 9.20.13
------------

New Features
~~~~~~~~~~~~

- Add manual mode configuration option to dnsec-policy. ``1e435b107f``

  Add a new option ``manual-mode`` to :any:`dnssec-policy`. The intended
  use is that if it is enabled, it will not automatically move to the
  next state transition, but instead the transition is logged. Only
  after manual confirmation with ``rndc dnssec -step`` the transition is
  made. :gl:`#4606` :gl:`!10880`

- Add a new 'servfail-until-ready' configuration option for RPZ.
  ``925af17d21``

  By default, when :iscman:`named` is started it may start answering to
  queries before the response policy zones are completely loaded and
  processed. This new feature gives an option to the users to tell
  :iscman:`named` that incoming requests should result in SERVFAIL
  answer until all the response policy zones are processed and ready.
  Note that if one or more response policy zones fail to load,
  :iscman:`named` starts responding to queries according to those zones
  that did load.

  Note, that enabling this option has no effect when a DNS Response
  Policy Service (DNSRPS) interface is used. :gl:`#5222` :gl:`!10889`

- Support for parsing HHIT and BRID records has been added.
  ``1f051af24d``

  :gl:`#5444` :gl:`!10932`

Removed Features
~~~~~~~~~~~~~~~~

- Deprecate the "tkey-gssapi-credential" statement. ``b239a70cac``

  The :any:`tkey-gssapi-keytab` statement allows GSS-TSIG to be set up
  in a simpler and more reliable way than using the
  :any:`tkey-gssapi-credential` statement and setting environment
  variables (e.g. ``KRB5_KTNAME``). Therefore, the
  :any:`tkey-gssapi-credential` statement has been deprecated;
  :any:`tkey-gssapi-keytab` should be used instead.

  For configurations currently using a combination of both
  :any:`tkey-gssapi-keytab` *and* :any:`tkey-gssapi-credential`, the
  latter should be dropped and the keytab pointed to by
  :any:`tkey-gssapi-keytab` should now only contain the credential
  previously specified by :any:`tkey-gssapi-credential`. :gl:`#4204`
  :gl:`!10924`

- Obsolete the "tkey-domain" statement. ``9352ae65d7``

  Mark the ``tkey-domain`` statement as obsolete, since it has not had
  any effect on server behavior since support for TKEY Mode 2
  (Diffie-Hellman) was removed (in BIND 9.20.0). :gl:`#4204`
  :gl:`!10926`

Feature Changes
~~~~~~~~~~~~~~~

- Update clang-format style with options added in newer versions.
  ``0c2c477c31``

  Add and apply InsertBraces statement to add missing curly braces
  around one-line statements and use
  ControlStatementsExceptControlMacros for SpaceBeforeParens to remove
  space between foreach macro and the brace, e.g. `FOREACH (x) {`
  becomes `FOREACH(x) {`. :gl:`!10864`

Bug Fixes
~~~~~~~~~

- Ensure file descriptors 0-2 are in use. ``35dee6eb90``

  libuv expect file descriptors <= STDERR_FILENO are in use. otherwise,
  it may abort when closing a file descriptor it opened. :gl:`#5226`
  :gl:`!10908`

- Prevent spurious SERVFAILs for certain 0-TTL resource records.
  ``6b266b222c``

  Under certain circumstances, BIND 9 can return SERVFAIL when updating
  existing entries in the cache with new NS, A, AAAA, or DS records with
  0-TTL. :gl:`#5294` :gl:`!10898`

- Use DNS_RDATACOMMON_INIT to hide branch differences. ``a64df9729b``

  Initialization of the common members of rdata type structures varies
  across branches. Standardize it by using the `DNS_RDATACOMMON_INIT`
  macro for all types, so that new types are more likely to use it, and
  hence backport more cleanly. :gl:`#5467` :gl:`!10834`

- RPZ canonical warning displays zone entry incorrectly. ``d833676515``

  When an IPv6 rpz prefix entry is entered incorrectly the log message
  was just displaying the prefix rather than the full entry.  This has
  been corrected. :gl:`#5491` :gl:`!10930`

- Fix a catalog zone issue when having an unset 'default-primaries'
  configuration clause. ``293e75af28``

  A catalog zone with an unset ``default-primaries`` clause could cause
  an unexpected termination of the :iscman:`named` process after two
  reloading or reconfiguration commands. This has been fixed.
  :gl:`#5494` :gl:`!10905`

- Add and use __attribute__((nonnull)) in dnssec-signzone.c.
  ``a8eed36d3e``

  Clang 20 was spuriously warning about the possibility of passing a
  NULL file pointer to `fprintf()`, which uses the 'nonnull' attribute.
  To silence the warning, the functions calling `fprintf()` have been
  marked with the same attribute to assure that NULL can't be passed to
  them in the first place.

  Close #5487 :gl:`!10913`

- RPZ 'servfail-until-ready': skip updating SERVFAIL cache.
  ``af2fb26325``

  In order to not pollute the SERVFAIL cache with the configured
  SERVFAIL answers while RPZ is loading, set the NS_CLIENTATTR_NOSETFC
  attribute for the client. :gl:`!10940`


