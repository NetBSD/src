#!/usr/bin/perl -w

# Apache $apr1$ to OpenLDAP {APR1} hash converter
# (C) 2011 Devin J. Pohly
# You may use this code freely.  It would be nice to be credited.

use MIME::Base64;

while (<>) {
	($user, $hash) = split(/:/, $_);
	unless ($hash =~ /^\$apr1\$/) {
		print STDERR "Not an Apache MD5 hash\n";
		exit 1;
	}

	chomp $hash;
	($_,$_,$salt,$hash) = split(/\$/, $hash);

	$hash =~ tr|./0-9A-Za-z|A-Za-z0-9+/|;
	$hash .= "AA";
	$hash =~ s/(.)(.)(.)(.)/$4$3$2$1/gs;
	$hash = decode_base64($hash);
	$hash =~ s/(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)..(.)/$1$4$7$10$13$15$2$5$8$11$14$16$3$6$9$12/s;
	$hash .= $salt;
	$hash = encode_base64($hash);
	chop $hash;

	print "$user:{APR1}$hash\n";
}