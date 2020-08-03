.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

..
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")

   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.

   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.


.. highlight: console

.. _man_dnssec-verify:

dnssec-verify - DNSSEC zone verification tool
---------------------------------------------

Synopsis
~~~~~~~~

:program:`dnssec-verify` [**-c** class] [**-E** engine] [**-I** input-format] [**-o** origin] [**-q**] [**-v** level] [**-V**] [**-x**] [**-z**] {zonefile}

Description
~~~~~~~~~~~

``dnssec-verify`` verifies that a zone is fully signed for each
algorithm found in the DNSKEY RRset for the zone, and that the NSEC /
NSEC3 chains are complete.

Options
~~~~~~~

**-c** class
   Specifies the DNS class of the zone.

**-E** engine
   Specifies the cryptographic hardware to use, when applicable.

   When BIND is built with OpenSSL PKCS#11 support, this defaults to the
   string "pkcs11", which identifies an OpenSSL engine that can drive a
   cryptographic accelerator or hardware service module. When BIND is
   built with native PKCS#11 cryptography (--enable-native-pkcs11), it
   defaults to the path of the PKCS#11 provider library specified via
   "--with-pkcs11".

**-I** input-format
   The format of the input zone file. Possible formats are ``"text"``
   (default) and ``"raw"``. This option is primarily intended to be used
   for dynamic signed zones so that the dumped zone file in a non-text
   format containing updates can be verified independently. The use of
   this option does not make much sense for non-dynamic zones.

**-o** origin
   The zone origin. If not specified, the name of the zone file is
   assumed to be the origin.

**-v** level
   Sets the debugging level.

**-V**
   Prints version information.

``-q``
   Quiet mode: Suppresses output.  Without this option, when ``dnssec-verify``
   is run it will print to standard output the number of keys in use, the
   algorithms used to verify the zone was signed correctly and other status
   information.  With it, all non-error output is suppressed, and only the exit
   code will indicate success.

**-x**
   Only verify that the DNSKEY RRset is signed with key-signing keys.
   Without this flag, it is assumed that the DNSKEY RRset will be signed
   by all active keys. When this flag is set, it will not be an error if
   the DNSKEY RRset is not signed by zone-signing keys. This corresponds
   to the ``-x`` option in ``dnssec-signzone``.

**-z**
   Ignore the KSK flag on the keys when determining whether the zone if
   correctly signed. Without this flag it is assumed that there will be
   a non-revoked, self-signed DNSKEY with the KSK flag set for each
   algorithm and that RRsets other than DNSKEY RRset will be signed with
   a different DNSKEY without the KSK flag set.

   With this flag set, we only require that for each algorithm, there
   will be at least one non-revoked, self-signed DNSKEY, regardless of
   the KSK flag state, and that other RRsets will be signed by a
   non-revoked key for the same algorithm that includes the self-signed
   key; the same key may be used for both purposes. This corresponds to
   the ``-z`` option in ``dnssec-signzone``.

**zonefile**
   The file containing the zone to be signed.

See Also
~~~~~~~~

:manpage:`dnssec-signzone(8)`, BIND 9 Administrator Reference Manual, :rfc:`4033`.
