<!--
 - Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 -
 - This Source Code Form is subject to the terms of the Mozilla Public
 - License, v. 2.0. If a copy of the MPL was not distributed with this
 - file, You can obtain one at http://mozilla.org/MPL/2.0/.
 -
 - See the COPYRIGHT file distributed with this work for additional
 - information regarding copyright ownership.
-->
### DNS Privacy in BIND

This directory contains sample configuration files to enable BIND,
with Nginx as a TLS proxy, to provide DNS over TLS.

`named.conf` configures a validating recursive name server to listen
on the localhost address at port 8853.

`nginx.conf` configures a TLS proxy to listen on port 853 and
forward queries and responses to `named`.

For more information, please see
[https://dnsprivacy.org/wiki/](https://dnsprivacy.org/wiki/)
