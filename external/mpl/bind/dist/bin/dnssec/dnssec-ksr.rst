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

.. iscman:: dnssec-ksr
.. program:: dnssec-ksr
.. _man_dnssec-ksr:

dnssec-ksr - Create signed key response (SKR) files for offline KSK setups
--------------------------------------------------------------------------

Synopsis
~~~~~~~~

:program:`dnssec-ksr` [**-E** engine] [**-e** date/offset] [**-F**] [**-f** file] [**-h**] [**-i** date/offset] [**-K** directory] [**-k** policy] [**-l** file] [**-o**] [**-V**] [**-v** level] {command} {zone}

Description
~~~~~~~~~~~

The :program:`dnssec-ksr` can be used to issue several commands that are needed
to generate presigned RRsets for a zone where the private key file of the Key
Signing Key (KSK) is typically offline. This requires Zone Signing Keys
(ZSKs) to be pregenerated, and the DNSKEY, CDNSKEY, and CDS RRsets to be
already signed in advance.

The latter is done by creating Key Signing Requests (KSRs) that can be imported
to the environment where the KSK is available. Once there, this program can
create Signed Key Responses (SKRs) that can be loaded by an authoritative DNS
server.

Options
~~~~~~~

.. option:: -E engine

   This option specifies the cryptographic hardware to use, when applicable.

   When BIND 9 is built with OpenSSL, this needs to be set to the OpenSSL
   engine identifier that drives the cryptographic accelerator or
   hardware service module (usually ``pkcs11``).

.. option:: -e date/offset

   This option sets the end date for which keys or SKRs need to be generated
   (depending on the command).

.. option:: -F

   This options turns on FIPS (US Federal Information Processing Standards)
   mode if the underlying crytographic library supports running in FIPS
   mode.

.. option:: -f

   This option sets the SKR file to be signed when issuing a ``sign`` command.

.. option:: -h

   This option prints a short summary of the options and arguments to
   :program:`dnssec-ksr`.

.. option:: -i date/offset

   This option sets the start date for which keys or SKRs need to be generated
   (depending on the command).

.. option:: -K directory

   This option sets the directory in which the key files are to be read or
   written (depending on the command).

.. option:: -k policy

   This option sets the specific ``dnssec-policy`` for which keys need to
   be generated, or signed.

.. option:: -l file

   This option provides a configuration file that contains a ``dnssec-policy``
   statement (matching the policy set with :option:`-k`).

.. option:: -o

   Normally when pregenerating keys, ZSKs are created. When this option is
   set, create KSKs instead.

.. option:: -V

   This option prints version information.

.. option:: -v level

   This option sets the debugging level. Level 1 is intended to be usefully
   verbose for general users; higher levels are intended for developers.

``command``

   The KSR command to be executed. See below for the available commands.

``zone``

   The name of the zone for which the KSR command is being executed.

Commands
~~~~~~~~

.. option:: keygen

  Pregenerate a number of keys, given a DNSSEC policy and an interval. The
  number of generated keys depends on the interval and the key lifetime.

.. option:: request

  Create a Key Signing Request (KSR), given a DNSSEC policy and an interval.
  This will generate a file with a number of key bundles, where each bundle
  contains the currently published ZSKs (according to the timing metadata).

.. option:: sign

  Sign a Key Signing Request (KSR), given a DNSSEC policy and an interval,
  creating a Signed Key Response (SKR). This will add the corresponding DNSKEY,
  CDS, and CDNSKEY records for the KSK that is being used for signing.

Exit Status
~~~~~~~~~~~

The :program:`dnssec-ksr` command exits 0 on success, or non-zero if an error
occurred.

Examples
~~~~~~~~

When you need to generate ZSKs for the zone "example.com" for the next year,
given a ``dnssec-policy`` named "mypolicy":

::

    dnssec-ksr -i now -e +1y -k mypolicy -l named.conf keygen example.com

Creating a KSR for the same zone and period can be done with:

::

    dnssec-ksr -i now -e +1y -k mypolicy -l named.conf request example.com > ksr.txt

Typically you would now transfer the KSR to the system that has access to
the KSK.

Signing the KSR created above can be done with:

::

    dnssec-ksr -i now -e +1y -k kskpolicy -l named.conf -f ksr.txt sign example.com

Make sure that the DNSSEC parameters in ``kskpolicy`` match those
in ``mypolicy``.

See Also
~~~~~~~~

:iscman:`dnssec-keygen(8) <dnssec-keygen>`,
:iscman:`dnssec-signzone(8) <dnssec-signzone>`,
BIND 9 Administrator Reference Manual.
