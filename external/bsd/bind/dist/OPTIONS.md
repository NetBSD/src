<!--
 - Copyright (C) 2017  Internet Systems Consortium, Inc. ("ISC")
 -
 - Permission to use, copy, modify, and/or distribute this software for any
 - purpose with or without fee is hereby granted, provided that the above
 - copyright notice and this permission notice appear in all copies.
 -
 - THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 - REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 - AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 - INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 - LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 - OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 - PERFORMANCE OF THIS SOFTWARE.
-->
Setting the `STD_CDEFINES` environment variable before running `configure`
can be used to enable certain compile-time options that are not explicitly
defined in `configure`.

Some of these settings are:

|Setting                            |Description |
|-----------------------------------|----------------------------------------|
|`-DISC_MEM_FILL=0`|Don't ovewrite memory when allocating or freeing it; this improves performance but makes debugging more difficult.|
|`-DISC_MEM_TRACKLINES=0`|Don't track memory allocations by file and line number; this improves performance but makes debugging more difficult.|
|<nobr>`-DISC_FACILITY=LOG_LOCAL0`</nobr>|Change the default syslog facility for `named`|
|`-DNS_CLIENT_DROPPORT=0`|Disable dropping queries from particular well-known ports:|
|`-DCHECK_SIBLING=0`|Don't check sibling glue in `named-checkzone`|
|`-DCHECK_LOCAL=0`|Don't check out-of-zone addresses in `named-checkzone`|
|`-DNS_RUN_PID_DIR=0`|Create default PID files in `${localstatedir}/run` rather than `${localstatedir}/run/{named,lwresd}/`|
|`-DDIG_SIGCHASE=1`|Enable DNSSEC signature chasing support in `dig`.  (Note: This feature is deprecated. Use `delv` instead.)|
|`-DNS_RPZ_MAX_ZONES=64`|Increase the maximum number of configurable response policy zones from 32 to 64; this is the highest possible setting|
|`-DISC_HEAP_CHECK`|Test heap consistency after every heap operation; used when debugging|
