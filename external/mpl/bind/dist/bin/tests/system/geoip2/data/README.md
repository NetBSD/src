Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.

##### Test MMDB databases

This directory contains test versions of the GeoIP2/GeoLite2 CIty,
Country, Domain, ISP, and ASN databases. The `.mmdb` files are built
from the corresponding `.json` source files; to regenerate them, modify
the source files and run `perl write-test-data.pl`.

This script is adapted from one in
[https://github.com/maxmind/MaxMind-DB](https://github.com/maxmind/MaxMind-DB).
It depends on the MaxMind:DB:Writer module, which can be found in
CPAN or at
[https://github.com/maxmind/MaxMind-DB-Writer-perl](https://github.com/maxmind/MaxMind-DB-Writer-perl) .
