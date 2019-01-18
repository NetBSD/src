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

# Compare two files, each with the output from dig, for differences.
# Ignore "unimportant" differences, like ordering of NS lines, TTL's,
# etc...

$lc = 0;
if ($ARGV[0] eq "--lc") {
	$lc = 1;
	shift;
}
$file1 = $ARGV[0];
$file2 = $ARGV[1];

$count = 0;
$firstname = "";
$status = 0;
$rcode1 = "none";
$rcode2 = "none";

open(FILE1, $file1) || die("open: $file1: $!\n");
while (<FILE1>) {
	~ s/\r\n//g;
	~ s/\n//g;
	if (/^;.+status:\s+(\S+).+$/) {
		$rcode1 = $1;
	}
	next if (/^;/);
	if (/^(\S+)\s+\S+\s+(\S+)\s+(\S+)\s+(.+)$/) {
		$name = $1;
		$class = $2;
		$type = $3;
		$value = $4;
		if ($lc) {
			$name = lc($name);
			$value = lc($value);
		}
		if ($type eq "SOA") {
			$firstname = $name if ($firstname eq "");
			if ($name eq $firstname) {
				$name = "$name$count";
				$count++;
			}
		}
		if ($entry{"$name ; $class.$type ; $value"} ne "") {
			$line = $entry{"$name ; $class.$type ; $value"};
			print("Duplicate entry in $file1:\n> $_\n< $line\n");
		} else {
			$entry{"$name ; $class.$type ; $value"} = $_;
		}
	} elsif (/^(\S+)\s+\S+\s+(\S+)\s+(\S+)\s*$/) {
		$name = $1;
		$class = $2;
		$type = $3;
		$value = "";
		if ($lc) {
			$name = lc($name);
			$value = lc($value);
		}
		if ($type eq "SOA") {
			$firstname = $name if ($firstname eq "");
			if ($name eq $firstname) {
				$name = "$name$count";
				$count++;
			}
		}
		if ($entry{"$name ; $class.$type ; $value"} ne "") {
			$line = $entry{"$name ; $class.$type ; $value"};
			print("Duplicate entry in $file1:\n> $_\n< $line\n");
		} else {
			$entry{"$name ; $class.$type ; $value"} = $_;
		}
	}
}
close(FILE1);

$printed = 0;

open(FILE2, $file2) || die("open: $file2: $!\n");
while (<FILE2>) {
	~ s/\r\n//g;
	~ s/\n//g;
	if (/^;.+status:\s+(\S+).+$/) {
		$rcode2 = $1;
	}
	next if (/^;/);
	if (/^(\S+)\s+\S+\s+(\S+)\s+(\S+)\s+(.+)$/) {
		$name = $1;
		$class = $2;
		$type = $3;
		$value = $4;
		if ($lc) {
			$name = lc($name);
			$value = lc($value);
		}
		if (($name eq $firstname) && ($type eq "SOA")) {
			$count--;
			$name = "$name$count";
		}
		if ($entry{"$name ; $class.$type ; $value"} ne "") {
			$entry{"$name ; $class.$type ; $value"} = "";
		} else {
			print("Only in $file2 (missing from $file1):\n")
			    if ($printed == 0);
			print("> $_\n");
			$printed++;
			$status = 1;
		}
	} elsif (/^(\S+)\s+\S+\s+(\S+)\s+(\S+)\s*$/) {
		$name = $1;
		$class = $2;
		$type = $3;
		$value = "";
		if ($lc) {
			$name = lc($name);
			$value = lc($value);
		}
		if (($name eq $firstname) && ($type eq "SOA")) {
			$count--;
			$name = "$name$count";
		}
		if ($entry{"$name ; $class.$type ; $value"} ne "") {
			$entry{"$name ; $class.$type ; $value"} = "";
		} else {
			print("Only in $file2 (missing from $file1):\n")
			    if ($printed == 0);
			print("> $_\n");
			$printed++;
			$status = 1;
		}
	}
}
close(FILE2);

$printed = 0;

foreach $key (keys(%entry)) {
	if ($entry{$key} ne "") {
		print("Only in $file1 (missing from $file2):\n")
		    if ($printed == 0);
		print("< $entry{$key}\n");
		$status = 1;
		$printed++;
	}
}

if ($rcode1 ne $rcode2) {
	print("< status: $rcode1\n");
	print("> status: $rcode2\n");
	$status = 1;
}

exit($status);
