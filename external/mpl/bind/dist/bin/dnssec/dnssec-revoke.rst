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

.. highlight: console

.. _man_dnssec-revoke:

dnssec-revoke - set the REVOKED bit on a DNSSEC key
---------------------------------------------------

Synopsis
~~~~~~~~

:program:`dnssec-revoke` [**-hr**] [**-v** level] [**-V**] [**-K** directory] [**-E** engine] [**-f**] [**-R**] {keyfile}

Description
~~~~~~~~~~~

``dnssec-revoke`` reads a DNSSEC key file, sets the REVOKED bit on the
key as defined in :rfc:`5011`, and creates a new pair of key files
containing the now-revoked key.

Options
~~~~~~~

``-h``
   This option emits a usage message and exits.

``-K directory``
   This option sets the directory in which the key files are to reside.

``-r``
   This option indicates to remove the original keyset files after writing the new keyset files.

``-v level``
   This option sets the debugging level.

``-V``
   This option prints version information.

``-E engine``
   This option specifies the cryptographic hardware to use, when applicable.

   When BIND 9 is built with OpenSSL, this needs to be set to the OpenSSL
   engine identifier that drives the cryptographic accelerator or
   hardware service module (usually ``pkcs11``). When BIND is
   built with native PKCS#11 cryptography (``--enable-native-pkcs11``), it
   defaults to the path of the PKCS#11 provider library specified via
   ``--with-pkcs11``.

``-f``
   This option indicates a forced overwrite and causes ``dnssec-revoke`` to write the new key pair,
   even if a file already exists matching the algorithm and key ID of
   the revoked key.

``-R``
   This option prints the key tag of the key with the REVOKE bit set, but does not
   revoke the key.

See Also
~~~~~~~~

:manpage:`dnssec-keygen(8)`, BIND 9 Administrator Reference Manual, :rfc:`5011`.
