#! @LOCALPREFIX@/bin/perl
#
#	$NetBSD: scriptdump.pl,v 1.2 1999/07/06 13:13:03 itojun Exp $
#

if ($< != 0) {
	print STDERR "must be root to invoke this\n";
	exit 1;
}

$mode = 'add';
while ($i = shift @ARGV) {
	if ($i eq '-d') {
		$mode = 'delete';
	} else {
		print STDERR "usage: scriptdump [-d]\n";
		exit 1;
	}
}

open(IN, "setkey -D |") || die;
foreach $_ (<IN>) {
	if (/^[^\t]/) {
		($src, $dst, $upper, $proxy) = split(/\s+/, $_);
	} elsif (/^\t(esp|ah) spi=(\d+).*replay=(\d+)/) {
		($proto, $spi, $replay) = ($1, $2, $3);
	} elsif (/^\tE: (\S+) (.*)/) {
		$ealgo = $1;
		$ekey = $2;
		$ekey =~ s/\s//g;
		$ekey =~ s/^/0x/g;
	} elsif (/^\tA: (\S+) (.*)/) {
		$aalgo = $1;
		$akey = $2;
		$akey =~ s/\s//g;
		$akey =~ s/^/0x/g;
	} elsif (/^\tstate=/) {
		print "$mode $src $src $upper $spi $proxy -p $proto";
		if ($mode eq 'add') {
			if ($proto eq 'esp') {
				print " -E $ealgo $ekey" if $ealgo;
				print " -A $aalgo $akey" if $aalgo;
			} elsif ($proto eq 'ah') {
				print " -A $aalgo $akey" if $aalgo;
			}
			print " -r $replay" if $replay;
		}
		print ";\n";

		$src = $dst = $upper = $proxy = '';
		$ealgo = $ekey = $aalgo = $akey = '';
	}
}
close(IN);

exit 0;
