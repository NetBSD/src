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
Setting the `STD_CDEFINES` environment variable before running `configure`
can be used to enable certain compile-time options that are not explicitly
defined in `configure`.

Some of these settings are:

|Setting                            |Description |
|-----------------------------------|----------------------------------------|
|`-DISC_MEM_DEFAULTFILL=1`|Overwrite memory with tag values when allocating or freeing it; this impairs performance but makes debugging of memory problems easier.|
|`-DISC_MEM_TRACKLINES=0`|Don't track memory allocations by file and line number; this improves performance but makes debugging more difficult.|
|<nobr>`-DISC_FACILITY=LOG_LOCAL0`</nobr>|Change the default syslog facility for `named`|
|`-DNS_CLIENT_DROPPORT=0`|Disable dropping queries from particular well-known ports:|
|`-DCHECK_SIBLING=0`|Don't check sibling glue in `named-checkzone`|
|`-DCHECK_LOCAL=0`|Don't check out-of-zone addresses in `named-checkzone`|
|`-DNS_RUN_PID_DIR=0`|Create default PID files in `${localstatedir}/run` rather than `${localstatedir}/run/named/`|
|`-DNS_RPZ_MAX_ZONES=64`|Increase the maximum number of configurable response policy zones from 32 to 64; this is the highest possible setting|
|`-DISC_BUFFER_USEINLINE=0`|Disable the use of inline functions to implement the `isc_buffer` API: this reduces performance but may be useful when debugging |
|`-DISC_HEAP_CHECK`|Test heap consistency after every heap operation; used when debugging|
