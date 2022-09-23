#!/usr/bin/perl

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

use warnings;
use strict;

if (@ARGV < 2) {
	print STDERR <<'END';
usage:
    perl docbook-options.pl options_file section > section.grammar.xml
END
	exit 1;
}

my $FILE = shift;
my $SECTION = shift;

open (FH, "<", $FILE) or die "Can't open $FILE";

print <<END;
.. Copyright (C) Internet Systems Consortium, Inc. ("ISC")
..
.. SPDX-License-Identifier: MPL-2.0
..
.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0.  If a copy of the MPL was not distributed with this
.. file, you can obtain one at https://mozilla.org/MPL/2.0/.
..
.. See the COPYRIGHT file distributed with this work for additional
.. information regarding copyright ownership.

::

END

# skip preamble
my $preamble = 0;
while (<FH>) {
	if (m{^\s*$}) {
		last if $preamble > 0;
	} else {
		$preamble++;
	}
}

my $display = 0;
while (<FH>) {
	if (m{^$SECTION\b}) {
		$display = 1
	}

	if (m{// not.*implemented} || m{// obsolete} ||
	    m{// ancient} || m{// test.*only})
	{
		next;
	}

	s{ // not configured}{};
	s{ // non-operational}{};
	s{ // may occur multiple times}{};
	s{[[]}{[}g;
	s{[]]}{]}g;
	s{        }{\t}g;

	if (m{^\s*$} && $display) {
		last;
	}
	if ($display) {
		print "  " . $_;
	}
}
