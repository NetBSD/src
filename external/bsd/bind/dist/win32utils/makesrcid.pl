#!/usr/bin/perl
#
# Copyright (C) 2012  Internet Systems Consortium, Inc. ("ISC")
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

# Id 

# This script converts the SRCID information in ../srcid into a srcid.h
# file, defining SRCID, which can be included by config.h.

open (SRCIDH, ">../srcid.h") or die "cannot open srcid.h: $!";

my $srcid = "unset";

if (open (SRCIDFILE, "../srcid")) {
    LOOP: while (<SRCIDFILE>) {
	chomp;
	($data) = split(/\#/);
	if($data) {
		($name, $value) = split(/=/,$data);
		($name) = split(/\s+/, $name);
		($value) = split(/\s+/, $value);
                next LOOP if ($name != "SRCID");
		$srcid = $value;
	}
    }
    close(SRCIDFILE);
}

# Now set up the output version file

$ThisDate = scalar localtime();

#Standard Header

print SRCIDH '/*
 * Copyright (C) 2012  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * INTERNET SOFTWARE CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

';

print SRCIDH "/*\n";
print SRCIDH " * srcid.h";
print SRCIDH " * Generated automatically by makesrcid.pl.\n";
print SRCIDH " * Date generated: $ThisDate\n";
print SRCIDH " */\n\n";

print SRCIDH '
#ifndef  SRCID_H
#define SRCID_H 1
';

print "BIND SRCID: $srcid\n";

print SRCIDH "#define SRCID\t\"$srcid\"\n";
print SRCIDH "#endif /* SRCID_H */\n";
close SRCIDH;
