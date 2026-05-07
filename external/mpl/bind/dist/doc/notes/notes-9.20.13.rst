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

Notes for BIND 9.20.13
----------------------

New Features
~~~~~~~~~~~~

- Add a new option ``manual-mode`` to :any:`dnssec-policy`.

  When enabled, :iscman:`named` will not modify DNSSEC keys or key states
  automatically. The proposed change will be logged and only after manual
  confirmation with ``rndc dnssec -step`` will the modification be made.
  :gl:`#4606`

- Add a new option ``servfail-until-ready`` to :namedconf:ref:`response-policy`
  zones.

  By default, when :iscman:`named` is started, it starts answering
  queries before all response policy zones are completely loaded and
  processed. This new option instructs :iscman:`named` to respond with
  SERVFAIL until all the response policy zones are processed and ready.
  Note that if one or more response policy zones fail to load,
  :iscman:`named` starts responding to queries according to those zones
  that did load.

  Note, that enabling this option has no effect when a DNS Response
  Policy Service (DNSRPS) interface is used. :gl:`#5222`

- Support for parsing HHIT and BRID records has been added.

  :gl:`#5444`

Removed Features
~~~~~~~~~~~~~~~~

- Deprecate the :namedconf:ref:`tkey-gssapi-credential` statement.

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

- Obsolete the "tkey-domain" statement.

  Mark the ``tkey-domain`` statement as obsolete because it has not had
  any effect on server behavior since support for TKEY Mode 2
  (Diffie-Hellman) was removed (in BIND 9.20.0). :gl:`#4204`

Bug Fixes
~~~~~~~~~

- Prevent spurious SERVFAILs for certain 0-TTL resource records.

  Under certain circumstances, BIND 9 can return SERVFAIL when updating
  existing entries in the cache with new NS, A, AAAA, or DS records that have a
  TTL of zero. :gl:`#5294`

- Fix unexpected termination if :namedconf:ref:`catalog-zones` had undefined
  ``default-primaries``.

  The issue manifested only if the server was reloaded or reconfigured twice.
  :gl:`#5494`


