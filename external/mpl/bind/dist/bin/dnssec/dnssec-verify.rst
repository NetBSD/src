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

.. iscman:: dnssec-verify
.. program:: dnssec-verify
.. _man_dnssec-verify:

dnssec-verify - DNSSEC zone verification tool
---------------------------------------------

Synopsis
~~~~~~~~

:program:`dnssec-verify` [**-c** class] [**-E** engine] [**-I** input-format] [**-o** origin] [**-q**] [**-v** level] [**-V**] [**-x**] [**-z**] {zonefile}

Description
~~~~~~~~~~~

:program:`dnssec-verify` verifies that a zone is fully signed for each
algorithm found in the DNSKEY RRset for the zone, and that the
NSEC/NSEC3 chains are complete.

Options
~~~~~~~

.. option:: -c class

   This option specifies the DNS class of the zone.

.. option:: -E engine

   This option specifies the cryptographic hardware to use, when applicable.

   When BIND 9 is built with OpenSSL, this needs to be set to the OpenSSL
   engine identifier that drives the cryptographic accelerator or
   hardware service module (usually ``pkcs11``).

.. option:: -I input-format

   This option sets the format of the input zone file. Possible formats are ``text``
   (the default) and ``raw``. This option is primarily intended to be used
   for dynamic signed zones, so that the dumped zone file in a non-text
   format containing updates can be verified independently.
   This option is not useful for non-dynamic zones.

.. option:: -o origin

   This option indicates the zone origin. If not specified, the name of the zone file is
   assumed to be the origin.

.. option:: -v level

   This option sets the debugging level.

.. option:: -V

   This option prints version information.

.. option:: -q

   This option sets quiet mode, which suppresses output.  Without this option, when :program:`dnssec-verify`
   is run it prints to standard output the number of keys in use, the
   algorithms used to verify the zone was signed correctly, and other status
   information.  With this option, all non-error output is suppressed, and only the exit
   code indicates success.

.. option:: -x

   This option verifies only that the DNSKEY RRset is signed with key-signing keys.
   Without this flag, it is assumed that the DNSKEY RRset is signed
   by all active keys. When this flag is set, it is not an error if
   the DNSKEY RRset is not signed by zone-signing keys. This corresponds
   to the :option:`-x option in dnssec-signzone <dnssec-signzone -x>`.

.. option:: -z

   This option indicates that the KSK flag on the keys should be ignored when determining whether the zone is
   correctly signed. Without this flag, it is assumed that there is
   a non-revoked, self-signed DNSKEY with the KSK flag set for each
   algorithm, and that RRsets other than DNSKEY RRset are signed with
   a different DNSKEY without the KSK flag set.

   With this flag set, BIND 9 only requires that for each algorithm, there
   be at least one non-revoked, self-signed DNSKEY, regardless of
   the KSK flag state, and that other RRsets be signed by a
   non-revoked key for the same algorithm that includes the self-signed
   key; the same key may be used for both purposes. This corresponds to
   the :option:`-z option in dnssec-signzone <dnssec-signzone -z>`.

.. option:: zonefile

   This option indicates the file containing the zone to be signed.

See Also
~~~~~~~~

:iscman:`dnssec-signzone(8) <dnssec-signzone>`, BIND 9 Administrator Reference Manual, :rfc:`4033`.
