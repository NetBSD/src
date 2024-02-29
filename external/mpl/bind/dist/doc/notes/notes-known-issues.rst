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

.. _relnotes_known_issues:

Known Issues
------------

- Upgrading from BIND 9.16.32, 9.18.6, or any older version may require
  a manual configuration change. The following configurations are
  affected:

  - :any:`type primary` zones configured with :any:`dnssec-policy` but
    without either :any:`allow-update` or :any:`update-policy`,
  - :any:`type secondary` zones configured with :any:`dnssec-policy`.

  In these cases please add :namedconf:ref:`inline-signing yes;
  <inline-signing>` to the individual zone configuration(s). Without
  applying this change, :iscman:`named` will fail to start. For more
  details, see
  https://kb.isc.org/docs/dnssec-policy-requires-dynamic-dns-or-inline-signing

- BIND 9.18 does not support dynamic update forwarding (see
  :any:`allow-update-forwarding`) in conjuction with zone transfers over
  TLS (XoT). :gl:`#3512`

- According to :rfc:`8310`, Section 8.1, the ``Subject`` field MUST NOT
  be inspected when verifying a remote certificate while establishing a
  DNS-over-TLS connection. Only ``subjectAltName`` must be checked
  instead. Unfortunately, some quite old versions of cryptographic
  libraries might lack the ability to ignore the ``Subject`` field. This
  should have minimal production-use consequences, as most of the
  production-ready certificates issued by certificate authorities will
  have ``subjectAltName`` set. In such cases, the ``Subject`` field is
  ignored. Only old platforms are affected by this, e.g. those supplied
  with OpenSSL versions older than 1.1.1. :gl:`#3163`

- ``rndc`` has been updated to use the new BIND network manager API. As
  the network manager currently has no support for UNIX-domain sockets,
  those cannot now be used with ``rndc``. This will be addressed in a
  future release, either by restoring UNIX-domain socket support or by
  formally declaring them to be obsolete in the control channel.
  :gl:`#1759`

- Sending NOTIFY messages silently fails when the source port specified
  in the :any:`notify-source` statement is already in use. This can
  happen e.g. when multiple servers are configured as NOTIFY targets for
  a zone and some of them are unresponsive. This issue can be worked
  around by not specifying the source port for NOTIFY messages in the
  :any:`notify-source` statement; note that source port configuration is
  already `deprecated`_ and will be removed altogether in a future
  release. :gl:`#4002`

.. _deprecated: https://gitlab.isc.org/isc-projects/bind9/-/issues/3781
