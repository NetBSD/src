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

.. _pkcs11:

PKCS#11 (Cryptoki) Support
--------------------------

Public Key Cryptography Standard #11 (PKCS#11) defines a
platform-independent API for the control of hardware security modules
(HSMs) and other cryptographic support devices.

BIND 9 is known to work with three HSMs: the AEP Keyper, which has been
tested with Debian Linux, Solaris x86, and Windows Server 2003; the
Thales nShield, tested with Debian Linux; and the Sun SCA 6000
cryptographic acceleration board, tested with Solaris x86. In addition,
BIND can be used with all current versions of SoftHSM, a software-based
HSM simulator library produced by the OpenDNSSEC project.

PKCS#11 uses a "provider library": a dynamically loadable
library which provides a low-level PKCS#11 interface to drive the HSM
hardware. The PKCS#11 provider library comes from the HSM vendor, and it
is specific to the HSM to be controlled.

There are two available mechanisms for PKCS#11 support in BIND 9:
OpenSSL-based PKCS#11 and native PKCS#11. With OpenSSL-based PKCS#11,
BIND uses a modified version of OpenSSL, which loads the
provider library and operates the HSM indirectly; any cryptographic
operations not supported by the HSM can be carried out by OpenSSL
instead. Native PKCS#11 enables BIND to bypass OpenSSL completely;
BIND loads the provider library itself, and uses the PKCS#11 API to
drive the HSM directly.

Prerequisites
~~~~~~~~~~~~~

See the documentation provided by the HSM vendor for information about
installing, initializing, testing, and troubleshooting the HSM.

Native PKCS#11
~~~~~~~~~~~~~~

Native PKCS#11 mode only works with an HSM capable of carrying out
*every* cryptographic operation BIND 9 may need. The HSM's provider
library must have a complete implementation of the PKCS#11 API, so that
all these functions are accessible. As of this writing, only the Thales
nShield HSM and SoftHSMv2 can be used in this fashion. For other HSMs,
including the AEP Keyper, Sun SCA 6000, and older versions of SoftHSM,
use OpenSSL-based PKCS#11. (Note: Eventually, when more HSMs become
capable of supporting native PKCS#11, it is expected that OpenSSL-based
PKCS#11 will be deprecated.)

To build BIND with native PKCS#11, configure it as follows:

::

   $ cd bind9
   $ ./configure --enable-native-pkcs11 \
       --with-pkcs11=provider-library-path


This causes all BIND tools, including ``named`` and the ``dnssec-*``
and ``pkcs11-*`` tools, to use the PKCS#11 provider library specified in
provider-library-path for cryptography. (The provider library path can
be overridden using the ``-E`` argument in ``named`` and the ``dnssec-*`` tools,
or the ``-m`` argument in the ``pkcs11-*`` tools.)

Building SoftHSMv2
^^^^^^^^^^^^^^^^^^

SoftHSMv2, the latest development version of SoftHSM, is available from
https://github.com/opendnssec/SoftHSMv2. It is a software library
developed by the OpenDNSSEC project (https://www.opendnssec.org) which
provides a PKCS#11 interface to a virtual HSM, implemented in the form
of an SQLite3 database on the local filesystem. It provides less security
than a true HSM, but it allows users to experiment with native PKCS#11
when an HSM is not available. SoftHSMv2 can be configured to use either
OpenSSL or the Botan library to perform cryptographic functions, but
when using it for native PKCS#11 in BIND, OpenSSL is required.

By default, the SoftHSMv2 configuration file is ``prefix/etc/softhsm2.conf``
(where ``prefix`` is configured at compile time). This location can be
overridden by the SOFTHSM2_CONF environment variable. The SoftHSMv2
cryptographic store must be installed and initialized before using it
with BIND.

::

   $  cd SoftHSMv2
   $  configure --with-crypto-backend=openssl --prefix=/opt/pkcs11/usr
   $  make
   $  make install
   $  /opt/pkcs11/usr/bin/softhsm-util --init-token 0 --slot 0 --label softhsmv2


OpenSSL-based PKCS#11
~~~~~~~~~~~~~~~~~~~~~

OpenSSL-based PKCS#11 uses engine_pkcs11 OpenSSL engine from libp11 project.

For more information, see https://gitlab.isc.org/isc-projects/bind9/-/wikis/BIND-9-PKCS11

PKCS#11 Tools
~~~~~~~~~~~~~

BIND 9 includes a minimal set of tools to operate the HSM, including
``pkcs11-keygen`` to generate a new key pair within the HSM,
``pkcs11-list`` to list objects currently available, ``pkcs11-destroy``
to remove objects, and ``pkcs11-tokens`` to list available tokens.

In UNIX/Linux builds, these tools are built only if BIND 9 is configured
with the ``--with-pkcs11`` option. (Note: If ``--with-pkcs11`` is set to ``yes``,
rather than to the path of the PKCS#11 provider, the tools are
built but the provider is left undefined. Use the ``-m`` option or the
``PKCS11_PROVIDER`` environment variable to specify the path to the
provider.)

Using the HSM
~~~~~~~~~~~~~

For OpenSSL-based PKCS#11, the runtime environment must first be set up
so the OpenSSL and PKCS#11 libraries can be loaded:

::

   $ export LD_LIBRARY_PATH=/opt/pkcs11/usr/lib:${LD_LIBRARY_PATH}

This causes ``named`` and other binaries to load the OpenSSL library
from ``/opt/pkcs11/usr/lib``, rather than from the default location. This
step is not necessary when using native PKCS#11.

Some HSMs require other environment variables to be set. For example,
when operating an AEP Keyper, the location of
the "machine" file, which stores information about the Keyper for use by
the provider library, must be specified. If the machine file is in
``/opt/Keyper/PKCS11Provider/machine``, use:

::

   $ export KEYPER_LIBRARY_PATH=/opt/Keyper/PKCS11Provider

Such environment variables must be set when running any tool that
uses the HSM, including ``pkcs11-keygen``, ``pkcs11-list``,
``pkcs11-destroy``, ``dnssec-keyfromlabel``, ``dnssec-signzone``,
``dnssec-keygen``, and ``named``.

HSM keys can now be created and used. In this case, we will create
a 2048-bit key and give it the label "sample-ksk":

::

   $ pkcs11-keygen -b 2048 -l sample-ksk

To confirm that the key exists:

::

   $ pkcs11-list
   Enter PIN:
   object[0]: handle 2147483658 class 3 label[8] 'sample-ksk' id[0]
   object[1]: handle 2147483657 class 2 label[8] 'sample-ksk' id[0]

Before using this key to sign a zone, we must create a pair of BIND 9
key files. The ``dnssec-keyfromlabel`` utility does this. In this case, we
are using the HSM key "sample-ksk" as the key-signing key for
"example.net":

::

   $ dnssec-keyfromlabel -l sample-ksk -f KSK example.net

The resulting K*.key and K*.private files can now be used to sign the
zone. Unlike normal K\* files, which contain both public and private key
data, these files contain only the public key data, plus an
identifier for the private key which remains stored within the HSM.
Signing with the private key takes place inside the HSM.

To generate a second key in the HSM for use as a
zone-signing key, follow the same procedure above, using a different
keylabel, a smaller key size, and omitting ``-f KSK`` from the
``dnssec-keyfromlabel`` arguments:

::

   $ pkcs11-keygen -b 1024 -l sample-zsk
   $ dnssec-keyfromlabel -l sample-zsk example.net

Alternatively, a conventional on-disk key can be generated
using ``dnssec-keygen``:

::

   $ dnssec-keygen example.net

This provides less security than an HSM key, but since HSMs can be slow
or cumbersome to use for security reasons, it may be more efficient to
reserve HSM keys for use in the less frequent key-signing operation. The
zone-signing key can be rolled more frequently, if desired, to
compensate for a reduction in key security. (Note: When using native
PKCS#11, there is no speed advantage to using on-disk keys, as
cryptographic operations are done by the HSM.)

Now the zone can be signed. Please note that, if the -S option is not used for
``dnssec-signzone``, the contents of both
``K*.key`` files must be added to the zone master file before signing it.

::

   $ dnssec-signzone -S example.net
   Enter PIN:
   Verifying the zone using the following algorithms:
   NSEC3RSASHA1.
   Zone signing complete:
   Algorithm: NSEC3RSASHA1: ZSKs: 1, KSKs: 1 active, 0 revoked, 0 stand-by
   example.net.signed

Specifying the Engine on the Command Line
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When using OpenSSL-based PKCS#11, the "engine" to be used by OpenSSL can
be specified in ``named`` and all of the BIND ``dnssec-*`` tools by
using the ``-E <engine>`` command line option. If BIND 9 is built with the
``--with-pkcs11`` option, this option defaults to "pkcs11". Specifying the
engine is generally not necessary unless
a different OpenSSL engine is used.

To disable use of the "pkcs11" engine - for
troubleshooting purposes, or because the HSM is unavailable - set
the engine to the empty string. For example:

::

   $ dnssec-signzone -E '' -S example.net

This causes ``dnssec-signzone`` to run as if it were compiled without
the ``--with-pkcs11`` option.

When built with native PKCS#11 mode, the "engine" option has a different
meaning: it specifies the path to the PKCS#11 provider library. This may
be useful when testing a new provider library.

Running ``named`` With Automatic Zone Re-signing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For ``named`` to dynamically re-sign zones using HSM keys,
and/or to sign new records inserted via nsupdate, ``named`` must
have access to the HSM PIN. In OpenSSL-based PKCS#11, this is
accomplished by placing the PIN into the ``openssl.cnf`` file (in the above
examples, ``/opt/pkcs11/usr/ssl/openssl.cnf``).

The location of the openssl.cnf file can be overridden by setting the
``OPENSSL_CONF`` environment variable before running ``named``.

Here is a sample ``openssl.cnf``:

::

       openssl_conf = openssl_def
       [ openssl_def ]
       engines = engine_section
       [ engine_section ]
       pkcs11 = pkcs11_section
       [ pkcs11_section ]
       PIN = <PLACE PIN HERE>

This also allows the ``dnssec-\*`` tools to access the HSM without PIN
entry. (The ``pkcs11-\*`` tools access the HSM directly, not via OpenSSL, so
a PIN is still required to use them.)

In native PKCS#11 mode, the PIN can be provided in a file specified as
an attribute of the key's label. For example, if a key had the label
``pkcs11:object=local-zsk;pin-source=/etc/hsmpin``, then the PIN would
be read from the file ``/etc/hsmpin``.

.. warning::

   Placing the HSM's PIN in a text file in this manner may reduce the
   security advantage of using an HSM. Use caution
   when configuring the system in this way.
