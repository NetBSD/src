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
