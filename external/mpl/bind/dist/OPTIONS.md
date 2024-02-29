<!--
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
-->
Setting the `CPPFLAGS` environment variable before running `configure`
can be used to enable certain compile-time options that are not
explicitly defined in `configure`.

Some of these settings are:

| Setting                      | Description                                                                                                                            |
| ---------------------------- | -------------------------------------------------------------------------------------------------------------------------------------- |
| `-DCHECK_LOCAL=0`            | Don't check out-of-zone addresses in `named-checkzone`                                                                                 |
| `-DCHECK_SIBLING=0`          | Don't check sibling glue in `named-checkzone`                                                                                          |
| `-DISC_FACILITY=LOG_LOCAL0`  | Change the default syslog facility for `named`                                                                                         |
| `-DISC_HEAP_CHECK`           | Test heap consistency after every heap operation; used when debugging                                                                  |
| `-DISC_MEM_DEFAULTFILL=1`    | Overwrite memory with tag values when allocating or freeing it; this impairs performance but makes debugging of memory problems easier |
| `-DISC_MEM_TRACKLINES=0`     | Don't track memory allocations by file and line number; this improves performance but makes debugging more difficult                   |
| `-DNAMED_RUN_PID_DIR=0`      | Create default PID files in `${localstatedir}/run` rather than `${localstatedir}/run/named/`                                           |
| `-DNS_CLIENT_DROPPORT=0`     | Disable dropping queries from particular well-known ports                                                                              |
| `-DOPENSSL_API_COMPAT=10100` | Build using the deprecated OpenSSL APIs so that the `engine` API is available when building with OpenSSL 3.0.0 for PKCS#11 support     |
