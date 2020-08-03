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

.. _pkcs11:

PKCS#11 (Cryptoki) support
--------------------------

PKCS#11 (Public Key Cryptography Standard #11) defines a
platform-independent API for the control of hardware security modules
(HSMs) and other cryptographic support devices.

BIND 9 is known to work with three HSMs: The AEP Keyper, which has been
tested with Debian Linux, Solaris x86 and Windows Server 2003; the
Thales nShield, tested with Debian Linux; and the Sun SCA 6000
cryptographic acceleration board, tested with Solaris x86. In addition,
BIND can be used with all current versions of SoftHSM, a software-based
HSM simulator library produced by the OpenDNSSEC project.

PKCS#11 makes use of a "provider library": a dynamically loadable
library which provides a low-level PKCS#11 interface to drive the HSM
hardware. The PKCS#11 provider library comes from the HSM vendor, and it
is specific to the HSM to be controlled.

There are two available mechanisms for PKCS#11 support in BIND 9:
OpenSSL-based PKCS#11 and native PKCS#11. When using the first
mechanism, BIND uses a modified version of OpenSSL, which loads the
provider library and operates the HSM indirectly; any cryptographic
operations not supported by the HSM can be carried out by OpenSSL
instead. The second mechanism enables BIND to bypass OpenSSL completely;
BIND loads the provider library itself, and uses the PKCS#11 API to
drive the HSM directly.

Prerequisites
~~~~~~~~~~~~~

See the documentation provided by your HSM vendor for information about
installing, initializing, testing and troubleshooting the HSM.

Native PKCS#11
~~~~~~~~~~~~~~

Native PKCS#11 mode will only work with an HSM capable of carrying out
*every* cryptographic operation BIND 9 may need. The HSM's provider
library must have a complete implementation of the PKCS#11 API, so that
all these functions are accessible. As of this writing, only the Thales
nShield HSM and SoftHSMv2 can be used in this fashion. For other HSMs,
including the AEP Keyper, Sun SCA 6000 and older versions of SoftHSM,
use OpenSSL-based PKCS#11. (Note: Eventually, when more HSMs become
capable of supporting native PKCS#11, it is expected that OpenSSL-based
PKCS#11 will be deprecated.)

To build BIND with native PKCS#11, configure as follows:

::

   $ cd bind9
   $ ./configure --enable-native-pkcs11 \
       --with-pkcs11=provider-library-path


This will cause all BIND tools, including ``named`` and the ``dnssec-*``
and ``pkcs11-*`` tools, to use the PKCS#11 provider library specified in
provider-library-path for cryptography. (The provider library path can
be overridden using the ``-E`` in ``named`` and the ``dnssec-*`` tools,
or the ``-m`` in the ``pkcs11-*`` tools.)

Building SoftHSMv2
^^^^^^^^^^^^^^^^^^

SoftHSMv2, the latest development version of SoftHSM, is available from
https://github.com/opendnssec/SoftHSMv2. It is a software library
developed by the OpenDNSSEC project (http://www.opendnssec.org) which
provides a PKCS#11 interface to a virtual HSM, implemented in the form
of a SQLite3 database on the local filesystem. It provides less security
than a true HSM, but it allows you to experiment with native PKCS#11
when an HSM is not available. SoftHSMv2 can be configured to use either
OpenSSL or the Botan library to perform cryptographic functions, but
when using it for native PKCS#11 in BIND, OpenSSL is required.

By default, the SoftHSMv2 configuration file is prefix/etc/softhsm2.conf
(where prefix is configured at compile time). This location can be
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

OpenSSL-based PKCS#11 mode uses a modified version of the OpenSSL
library; stock OpenSSL does not fully support PKCS#11. ISC provides a
patch to OpenSSL to correct this. This patch is based on work originally
done by the OpenSolaris project; it has been modified by ISC to provide
new features such as PIN management and key-by-reference.

There are two "flavors" of PKCS#11 support provided by the patched
OpenSSL, one of which must be chosen at configuration time. The correct
choice depends on the HSM hardware:

-  Use 'crypto-accelerator' with HSMs that have hardware cryptographic
   acceleration features, such as the SCA 6000 board. This causes
   OpenSSL to run all supported cryptographic operations in the HSM.

-  Use 'sign-only' with HSMs that are designed to function primarily as
   secure key storage devices, but lack hardware acceleration. These
   devices are highly secure, but are not necessarily any faster at
   cryptography than the system CPU MDASH often, they are slower. It is
   therefore most efficient to use them only for those cryptographic
   functions that require access to the secured private key, such as
   zone signing, and to use the system CPU for all other
   computationally-intensive operations. The AEP Keyper is an example of
   such a device.

The modified OpenSSL code is included in the BIND 9 release, in the form
of a context diff against the latest versions of OpenSSL. OpenSSL 0.9.8,
1.0.0, 1.0.1 and 1.0.2 are supported; there are separate diffs for each
version. In the examples to follow, we use OpenSSL 0.9.8, but the same
methods work with OpenSSL 1.0.0 through 1.0.2.

.. note::

   The OpenSSL patches as of this writing (January 2016) support
   versions 0.9.8zh, 1.0.0t, 1.0.1q and 1.0.2f. ISC will provide updated
   patches as new versions of OpenSSL are released. The version number
   in the following examples is expected to change.

Before building BIND 9 with PKCS#11 support, it will be necessary to
build OpenSSL with the patch in place, and configure it with the path to
your HSM's PKCS#11 provider library.

Patching OpenSSL
^^^^^^^^^^^^^^^^

::

   $ wget http://www.openssl.org/source/openssl-0.9.8zc.tar.gz


Extract the tarball:

::

   $ tar zxf openssl-0.9.8zc.tar.gz

Apply the patch from the BIND 9 release:

::

   $ patch -p1 -d openssl-0.9.8zc \
             < bind9/bin/pkcs11/openssl-0.9.8zc-patch

..

.. note::

   The patch file may not be compatible with the "patch" utility on all
   operating systems. You may need to install GNU patch.

When building OpenSSL, place it in a non-standard location so that it
does not interfere with OpenSSL libraries elsewhere on the system. In
the following examples, we choose to install into "/opt/pkcs11/usr". We
will use this location when we configure BIND 9.

Later, when building BIND 9, the location of the custom-built OpenSSL
library will need to be specified via configure.

Building OpenSSL for the AEP Keyper on Linux
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The AEP Keyper is a highly secure key storage device, but does not
provide hardware cryptographic acceleration. It can carry out
cryptographic operations, but it is probably slower than your system's
CPU. Therefore, we choose the 'sign-only' flavor when building OpenSSL.

The Keyper-specific PKCS#11 provider library is delivered with the
Keyper software. In this example, we place it /opt/pkcs11/usr/lib:

::

   $ cp pkcs11.GCC4.0.2.so.4.05 /opt/pkcs11/usr/lib/libpkcs11.so

::

   $ cd openssl-0.9.8zc
   $ ./Configure linux-x86_64 \
           --pk11-libname=/opt/pkcs11/usr/lib/libpkcs11.so \
           --pk11-flavor=sign-only \
           --prefix=/opt/pkcs11/usr

Building OpenSSL for the SCA 6000 on Solaris
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The SCA-6000 PKCS#11 provider is installed as a system library,
libpkcs11. It is a true crypto accelerator, up to 4 times faster than
any CPU, so the flavor shall be 'crypto-accelerator'.

In this example, we are building on Solaris x86 on an AMD64 system.

::

   $ cd openssl-0.9.8zc
   $ ./Configure solaris64-x86_64-cc \
           --pk11-libname=/usr/lib/64/libpkcs11.so \
           --pk11-flavor=crypto-accelerator \
           --prefix=/opt/pkcs11/usr

(For a 32-bit build, use "solaris-x86-cc" and /usr/lib/libpkcs11.so.)

After configuring, run ``make`` and ``make test``.

Building OpenSSL for SoftHSM
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

SoftHSM (version 1) is a software library developed by the OpenDNSSEC
project (http://www.opendnssec.org) which provides a PKCS#11 interface
to a virtual HSM, implemented in the form of a SQLite3 database on the
local filesystem. SoftHSM uses the Botan library to perform
cryptographic functions. Though less secure than a true HSM, it can
allow you to experiment with PKCS#11 when an HSM is not available.

The SoftHSM cryptographic store must be installed and initialized before
using it with OpenSSL, and the SOFTHSM_CONF environment variable must
always point to the SoftHSM configuration file:

::

   $  cd softhsm-1.3.7
   $  configure --prefix=/opt/pkcs11/usr
   $  make
   $  make install
   $  export SOFTHSM_CONF=/opt/pkcs11/softhsm.conf
   $  echo "0:/opt/pkcs11/softhsm.db" > $SOFTHSM_CONF
   $  /opt/pkcs11/usr/bin/softhsm --init-token 0 --slot 0 --label softhsm

SoftHSM can perform all cryptographic operations, but since it only uses
your system CPU, there is no advantage to using it for anything but
signing. Therefore, we choose the 'sign-only' flavor when building
OpenSSL.

::

   $ cd openssl-0.9.8zc
   $ ./Configure linux-x86_64 \
           --pk11-libname=/opt/pkcs11/usr/lib/libsofthsm.so \
           --pk11-flavor=sign-only \
           --prefix=/opt/pkcs11/usr

After configuring, run "``make``" and "``make test``".

Once you have built OpenSSL, run "``apps/openssl engine pkcs11``" to
confirm that PKCS#11 support was compiled in correctly. The output
should be one of the following lines, depending on the flavor selected:

::

       (pkcs11) PKCS #11 engine support (sign only)

Or:

::

       (pkcs11) PKCS #11 engine support (crypto accelerator)

Next, run "``apps/openssl engine pkcs11 -t``". This will attempt to
initialize the PKCS#11 engine. If it is able to do so successfully, it
will report “``[ available ]``”.

If the output is correct, run "``make install``" which will install the
modified OpenSSL suite to ``/opt/pkcs11/usr``.

Configuring BIND 9 for Linux with the AEP Keyper
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   $ cd ../bind9
   $ ./configure \
          --with-openssl=/opt/pkcs11/usr \
          --with-pkcs11=/opt/pkcs11/usr/lib/libpkcs11.so

Configuring BIND 9 for Solaris with the SCA 6000
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   $ cd ../bind9
   $ ./configure CC="cc -xarch=amd64" \
           --with-openssl=/opt/pkcs11/usr \
           --with-pkcs11=/usr/lib/64/libpkcs11.so

(For a 32-bit build, omit CC="cc -xarch=amd64".)

If configure complains about OpenSSL not working, you may have a
32/64-bit architecture mismatch. Or, you may have incorrectly specified
the path to OpenSSL (it should be the same as the --prefix argument to
the OpenSSL Configure).

Configuring BIND 9 for SoftHSM
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   $ cd ../bind9
   $ ./configure \
          --with-openssl=/opt/pkcs11/usr \
          --with-pkcs11=/opt/pkcs11/usr/lib/libsofthsm.so

After configuring, run "``make``", "``make test``" and
"``make install``".

(Note: If "make test" fails in the "pkcs11" system test, you may have
forgotten to set the SOFTHSM_CONF environment variable.)

PKCS#11 Tools
~~~~~~~~~~~~~

BIND 9 includes a minimal set of tools to operate the HSM, including
``pkcs11-keygen`` to generate a new key pair within the HSM,
``pkcs11-list`` to list objects currently available, ``pkcs11-destroy``
to remove objects, and ``pkcs11-tokens`` to list available tokens.

In UNIX/Linux builds, these tools are built only if BIND 9 is configured
with the --with-pkcs11 option. (Note: If --with-pkcs11 is set to "yes",
rather than to the path of the PKCS#11 provider, then the tools will be
built but the provider will be left undefined. Use the -m option or the
PKCS11_PROVIDER environment variable to specify the path to the
provider.)

Using the HSM
~~~~~~~~~~~~~

For OpenSSL-based PKCS#11, we must first set up the runtime environment
so the OpenSSL and PKCS#11 libraries can be loaded:

::

   $ export LD_LIBRARY_PATH=/opt/pkcs11/usr/lib:${LD_LIBRARY_PATH}

This causes ``named`` and other binaries to load the OpenSSL library
from ``/opt/pkcs11/usr/lib`` rather than from the default location. This
step is not necessary when using native PKCS#11.

Some HSMs require other environment variables to be set. For example,
when operating an AEP Keyper, it is necessary to specify the location of
the "machine" file, which stores information about the Keyper for use by
the provider library. If the machine file is in
``/opt/Keyper/PKCS11Provider/machine``, use:

::

   $ export KEYPER_LIBRARY_PATH=/opt/Keyper/PKCS11Provider

Such environment variables must be set whenever running any tool that
uses the HSM, including ``pkcs11-keygen``, ``pkcs11-list``,
``pkcs11-destroy``, ``dnssec-keyfromlabel``, ``dnssec-signzone``,
``dnssec-keygen``, and ``named``.

We can now create and use keys in the HSM. In this case, we will create
a 2048 bit key and give it the label "sample-ksk":

::

   $ pkcs11-keygen -b 2048 -l sample-ksk

To confirm that the key exists:

::

   $ pkcs11-list
   Enter PIN:
   object[0]: handle 2147483658 class 3 label[8] 'sample-ksk' id[0]
   object[1]: handle 2147483657 class 2 label[8] 'sample-ksk' id[0]

Before using this key to sign a zone, we must create a pair of BIND 9
key files. The "dnssec-keyfromlabel" utility does this. In this case, we
will be using the HSM key "sample-ksk" as the key-signing key for
"example.net":

::

   $ dnssec-keyfromlabel -l sample-ksk -f KSK example.net

The resulting K*.key and K*.private files can now be used to sign the
zone. Unlike normal K\* files, which contain both public and private key
data, these files will contain only the public key data, plus an
identifier for the private key which remains stored within the HSM.
Signing with the private key takes place inside the HSM.

If you wish to generate a second key in the HSM for use as a
zone-signing key, follow the same procedure above, using a different
keylabel, a smaller key size, and omitting "-f KSK" from the
dnssec-keyfromlabel arguments:

(Note: When using OpenSSL-based PKCS#11 the label is an arbitrary string
which identifies the key. With native PKCS#11, the label is a PKCS#11
URI string which may include other details about the key and the HSM,
including its PIN. See :ref:`man_dnssec-keyfromlabel` for details.)

::

   $ pkcs11-keygen -b 1024 -l sample-zsk
   $ dnssec-keyfromlabel -l sample-zsk example.net

Alternatively, you may prefer to generate a conventional on-disk key,
using dnssec-keygen:

::

   $ dnssec-keygen example.net

This provides less security than an HSM key, but since HSMs can be slow
or cumbersome to use for security reasons, it may be more efficient to
reserve HSM keys for use in the less frequent key-signing operation. The
zone-signing key can be rolled more frequently, if you wish, to
compensate for a reduction in key security. (Note: When using native
PKCS#11, there is no speed advantage to using on-disk keys, as
cryptographic operations will be done by the HSM regardless.)

Now you can sign the zone. (Note: If not using the -S option to
``dnssec-signzone``, it will be necessary to add the contents of both
``K*.key`` files to the zone master file before signing it.)

::

   $ dnssec-signzone -S example.net
   Enter PIN:
   Verifying the zone using the following algorithms:
   NSEC3RSASHA1.
   Zone signing complete:
   Algorithm: NSEC3RSASHA1: ZSKs: 1, KSKs: 1 active, 0 revoked, 0 stand-by
   example.net.signed

Specifying the engine on the command line
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When using OpenSSL-based PKCS#11, the "engine" to be used by OpenSSL can
be specified in ``named`` and all of the BIND ``dnssec-*`` tools by
using the "-E <engine>" command line option. If BIND 9 is built with the
--with-pkcs11 option, this option defaults to "pkcs11". Specifying the
engine will generally not be necessary unless for some reason you wish
to use a different OpenSSL engine.

If you wish to disable use of the "pkcs11" engine MDASH for
troubleshooting purposes, or because the HSM is unavailable MDASH set
the engine to the empty string. For example:

::

   $ dnssec-signzone -E '' -S example.net

This causes ``dnssec-signzone`` to run as if it were compiled without
the --with-pkcs11 option.

When built with native PKCS#11 mode, the "engine" option has a different
meaning: it specifies the path to the PKCS#11 provider library. This may
be useful when testing a new provider library.

Running named with automatic zone re-signing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you want ``named`` to dynamically re-sign zones using HSM keys,
and/or to to sign new records inserted via nsupdate, then ``named`` must
have access to the HSM PIN. In OpenSSL-based PKCS#11, this is
accomplished by placing the PIN into the openssl.cnf file (in the above
examples, ``/opt/pkcs11/usr/ssl/openssl.cnf``).

The location of the openssl.cnf file can be overridden by setting the
OPENSSL_CONF environment variable before running ``named``.

Sample openssl.cnf:

::

       openssl_conf = openssl_def
       [ openssl_def ]
       engines = engine_section
       [ engine_section ]
       pkcs11 = pkcs11_section
       [ pkcs11_section ]
       PIN = <PLACE PIN HERE>

This will also allow the dnssec-\* tools to access the HSM without PIN
entry. (The pkcs11-\* tools access the HSM directly, not via OpenSSL, so
a PIN will still be required to use them.)

In native PKCS#11 mode, the PIN can be provided in a file specified as
an attribute of the key's label. For example, if a key had the label
``pkcs11:object=local-zsk;pin-source=/etc/hsmpin``, then the PIN would
be read from the file ``/etc/hsmpin``.

.. warning::

   Placing the HSM's PIN in a text file in this manner may reduce the
   security advantage of using an HSM. Be sure this is what you want to
   do before configuring the system in this way.
