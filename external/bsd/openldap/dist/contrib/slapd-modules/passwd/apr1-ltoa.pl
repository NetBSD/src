#!/usr/bin/perl -w

# OpenLDAP {APR1} to Apache $apr1$ hash converter
# (C) 2011 Devin J. Pohly
# You may use this code freely.  It would be nice to be credited.

use MIME::Base64;

while (<>) {
	($user, $hash) = split(/:/, $_);
	unless ($hash =~ /^{APR1}/) {
		print STDERR "Not an Apache MD5 hash\n";
		next;
	}

	chomp $hash;
	$hash = decode_base64(substr($hash, 6));
	($hash, $salt) = (substr($hash, 0, 16), substr($hash, 16));
	$hash = $hash;
	$hash =~ s/(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)(.)/$1$7$13$2$8$14$3$9$15$4$10$16$5$11$6\0\0$12/s;
	$hash = encode_base64($hash);
	chomp $hash;
	$hash =~ s/(.)(.)(.)(.)/$4$3$2$1/gs;
	unless ($hash =~ /AA$/) {
		#print "Problem with hash\n";
		next;
	}
	$hash =~ s/AA$//;
	$hash =~ tr|A-Za-z0-9+/|./0-9A-Za-z|;
	print "$user:\$apr1\$$salt\$$hash\n"
}