#!/bin/sh

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

set -e

. ../conf.sh

# Branching factor equals DNS_RDATASET_MAXADDITIONAL so every RRset passes
# the per-RRset additional-processing limit.  PARENTS is the highest node
# index that still owns an HTTPS RRset; its children are empty leaves.
BRANCHES=13
PARENTS=182

{
  echo "\$TTL 43200"
  echo "@	IN SOA	ns2. hostmaster.ns2. ( 1 3600 1800 604800 86400 )"
  echo "@	IN NS	ns2."
} >ns2/tree.db

awk -v b="$BRANCHES" -v p="$PARENTS" 'END {
	for (i = 0; i <= p; i++) {
		for (j = 1; j <= b; j++) {
			printf "n%d\tIN HTTPS\t0 n%d\n", i, b * i + j
		}
	}
}' </dev/null >>ns2/tree.db
