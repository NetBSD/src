#!/usr/bin/perl -w

# Given a diff and a source tree, make local copies of all modified files
# in the source tree.  The patch is read from stdin, and spit back out
# to stdout.

use strict;

die "Usage: $0 <source tree>\n"	unless (@ARGV == 1);

foreach (<STDIN>) {

	if (/^diff\s+/) {
		my $file = (split /\s+/)[-1];
		my @temp = split /\/+/, $file;
		shift @temp;
		my $x = pop @temp;

		my $dir = join '/', @temp;
		push @temp, $x;
		$file = join '/', @temp;

		if (-e "$ARGV[0]/$file") {
			system ("mkdir -p $dir") == 0 ||
					die "Error: `mkdir -p $dir` failed.\n";
			system ("cp $ARGV[0]/$file $dir") == 0 ||
					die "Error: `cp $ARGV[0]$file $dir` failed.\n";
		}
		else {
			print STDERR "Warning: cannot find $ARGV[0]/$file.\n"
		}
	}

	print;
}
