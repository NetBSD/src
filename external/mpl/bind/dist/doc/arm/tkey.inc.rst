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

TKEY
----

TKEY (Transaction KEY) is a mechanism for automatically negotiating a
shared secret between two hosts, originally specified in :rfc:`2930`.

There are several TKEY "modes" that specify how a key is to be generated
or assigned. BIND 9 implements only one of these modes: Diffie-Hellman
key exchange. Both hosts are required to have a KEY record with
algorithm DH (though this record is not required to be present in a
zone).

The TKEY process is initiated by a client or server by sending a query
of type TKEY to a TKEY-aware server. The query must include an
appropriate KEY record in the additional section, and must be signed
using either TSIG or SIG(0) with a previously established key. The
server's response, if successful, contains a TKEY record in its
answer section. After this transaction, both participants have
enough information to calculate a shared secret using Diffie-Hellman key
exchange. The shared secret can then be used to sign subsequent
transactions between the two servers.

TSIG keys known by the server, including TKEY-negotiated keys, can be
listed using :option:`rndc tsig-list`.

TKEY-negotiated keys can be deleted from a server using
:option:`rndc tsig-delete`. This can also be done via the TKEY protocol
itself, by sending an authenticated TKEY query specifying the "key
deletion" mode.
