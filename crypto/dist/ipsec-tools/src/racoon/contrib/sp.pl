#! /usr/pkg/bin/perl

die "insufficient arguments" if (scalar(@ARGV) < 2);
$src = $ARGV[0];
$dst = $ARGV[1];
$mode = 'transport';
if (scalar(@ARGV) > 2) {
	$mode = $ARGV[2];
}

open(OUT, "|setkey -c");
if ($mode eq 'transport') {
	print STDERR "install esp transport mode: $src -> $dst\n";
	print OUT "spdadd $src $dst any -P out ipsec esp/transport//require;\n";
	print OUT "spdadd $dst $src any -P in ipsec esp/transport//require;\n";
} elsif ($mode eq 'delete') {
	print STDERR "delete policy: $src -> $dst\n";
	print OUT "spddelete $src $dst any -P out;\n";
	print OUT "spddelete $dst $src any -P in;\n";
}
close(OUT);
