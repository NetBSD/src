#!/usr/bin/perl
# usage:
# % cat /var/log/racoon-stats.log | perl stats.pl

while(<STDIN>) {
	chomp;
	($a, $a, $a, $a, $a, $b) = split(/\s+/, $_, 6);
	($a, $c) = split(/:/, $b, 2);
	$r{$a} += $c;
	$t{$a}++;
}

foreach (sort keys %t) {
	printf "%s: total=%d avg=%8.6f\n", $_, $t{$_}, $r{$_}/$t{$_};
}
