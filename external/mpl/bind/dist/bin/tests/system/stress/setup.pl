#!/usr/bin/perl
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

#
# Set up test data for zone transfer quota tests.
#
use FileHandle;

my $n_zones = 5;
my $n_names = 1000;

make_zones(2, undef);
make_zones(3, "10.53.0.2");
make_zones(4, "10.53.0.3");

my $rootdelegations =
    new FileHandle("ns1/root.db", "w") or die;

print $rootdelegations <<END;
\$TTL 300
.                       IN SOA  gson.nominum.com. a.root.servers.nil. (
					       2000042100      ; serial
					       600             ; refresh
					       600             ; retry
					       1200            ; expire
					       600             ; minimum
                                )
.                       NS      a.root-servers.nil.
a.root-servers.nil.     A       10.53.0.1
END

for ($z = 0; $z < $n_zones; $z++) {
	my $zn = sprintf("zone%06d.example", $z);
	foreach $ns (qw(2 3 4)) {
		print $rootdelegations "$zn.		NS	ns$ns.$zn.\n";
		print $rootdelegations "ns$ns.$zn.	A	10.53.0.$ns\n";		
	}
}
close $rootdelegations;
	
sub make_zones {
	my ($nsno, $slaved_from) = @_;
	my $namedconf = new FileHandle("ns$nsno/zones.conf", "w") or die;
	for ($z = 0; $z < $n_zones; $z++) {
		my $zn = sprintf("zone%06d.example", $z);
		if (defined($slaved_from)) {
			print $namedconf "zone \"$zn\" { type slave; " .
			    "file \"$zn.bk\"; masters { $slaved_from; }; };\n";
		} else {
			print $namedconf "zone \"$zn\" { " .
			    "type master; " .
			    "allow-update { any; }; " .
			    "file \"$zn.db\"; };\n";

			my $fn = "ns$nsno/$zn.db";
			my $f = new FileHandle($fn, "w") or die "open: $fn: $!";
			print $f "\$TTL 300
\@	IN SOA 	ns2.$zn. hostmaster 1 300 120 3600 86400
@	NS	ns2.$zn.
ns2.$zn.	A	10.53.0.2
@	NS	ns3.$zn.
ns3.$zn.	A	10.53.0.3
@	NS	ns4.$zn.
ns4.$zn.	A	10.53.0.4
	MX	10 mail1.isp.example.
	MX	20 mail2.isp.example.
";
			for ($i = 0; $i < $n_names; $i++) {
			    print $f sprintf("name%06d", $i) .
				"	A	10.0.0.1\n";
		    }
		    $f->close;
		}
	}
}
