.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

Notes for BIND 9.16.3
---------------------

Known Issues
~~~~~~~~~~~~

-  BIND crashes on startup when linked against libuv 1.36. This issue
   is related to ``recvmmsg()`` support in libuv, which was first
   included in libuv 1.35. The problem was addressed in libuv 1.37, but
   the relevant libuv code change requires a special flag to be set
   during library initialization in order for ``recvmmsg()`` support to
   be enabled. This BIND release sets that special flag when required,
   so ``recvmmsg()`` support is now enabled when BIND is compiled
   against either libuv 1.35 or libuv 1.37+; libuv 1.36 is still not
   usable with BIND. [GL #1761] [GL #1797]

Feature Changes
~~~~~~~~~~~~~~~

-  BIND 9 no longer sets receive/send buffer sizes for UDP sockets,
   relying on system defaults instead. [GL #1713]

-  The default rwlock implementation has been changed back to the native
   BIND 9 rwlock implementation. [GL #1753]

-  The native PKCS#11 EdDSA implementation has been updated to PKCS#11
   v3.0 and thus made operational again. Contributed by Aaron Thompson.
   [GL !3326]

-  The OpenSSL ECDSA implementation has been updated to support PKCS#11
   via OpenSSL engine (see engine_pkcs11 from libp11 project). [GL
   #1534]

-  The OpenSSL EdDSA implementation has been updated to support PKCS#11
   via OpenSSL engine. Please note that an EdDSA-capable OpenSSL engine
   is required and thus this code is only a proof-of-concept for the
   time being. Contributed by Aaron Thompson. [GL #1763]

-  Message IDs in inbound AXFR transfers are now checked for
   consistency. Log messages are emitted for streams with inconsistent
   message IDs. [GL #1674]

-  The zone timers are now exported to the statistics channel. For the
   primary zones, only the loaded time is exported. For the secondary
   zones, the exported timers also include expire and refresh times.
   Contributed by Paul Frieden, Verizon Media. [GL #1232]

Bug Fixes
~~~~~~~~~

-  A bug in dnstap initialization could prevent some dnstap data from
   being logged, especially on recursive resolvers. [GL #1795]

-  When running on a system with support for Linux capabilities,
   ``named`` drops root privileges very soon after system startup. This
   was causing a spurious log message, ``unable to set effective uid to
   0: Operation not permitted``, which has now been silenced. [GL #1042]
   [GL #1090]

-  When ``named-checkconf`` was run, it would sometimes incorrectly set
   its exit code. It reflected only the status of the last view found;
   any errors found for other configured views were not reported. Thanks
   to Graham Clinch. [GL #1807]

-  When built without LMDB support, ``named`` failed to restart after a
   zone with a double quote (") in its name was added with
   ``rndc addzone``. Thanks to Alberto Fernández. [GL #1695]
