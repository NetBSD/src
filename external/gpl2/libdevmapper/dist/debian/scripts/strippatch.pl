#!/usr/bin/perl -w

# Strip out non-header files from a diff.

use strict;

die "Usage: $0 <patchfile>\n"	unless (@ARGV == 1);

my $ctx = '';
open (F, $ARGV[0]) || die "Error: can't open $ARGV[0]: $!\n";
while (<F>) {

	if (/^diff\s+/) {
		if ($ctx) {
			print $ctx;
			$ctx = '';
		}

		if (/\/include\/.*\.h\s*$/) {
			$ctx = $_;
		}
	}
	elsif ($ctx) {
		$ctx .= $_;
	}
}
close (F);
