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

SIG(0)
------

BIND partially supports DNSSEC SIG(0) transaction signatures as
specified in :rfc:`2535` and :rfc:`2931`. SIG(0) uses public/private keys to
authenticate messages. Access control is performed in the same manner as with
TSIG keys; privileges can be granted or denied in ACL directives based
on the key name.

When a SIG(0) signed message is received, it is only verified if
the key is known and trusted by the server. The server does not attempt
to recursively fetch or validate the key.

SIG(0) signing of multiple-message TCP streams is not supported.

The only tool shipped with BIND 9 that generates SIG(0) signed messages
is :iscman:`nsupdate`.
