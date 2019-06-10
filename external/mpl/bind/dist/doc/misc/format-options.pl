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

print <<END;

This is a summary of the named.conf options supported by 
this version of BIND 9.

END

# Break long lines
while (<>) {
	chomp;
	s/\t/        /g;
	my $line = $_;
	m!^( *)!;
	my $indent = $1;
	my $comment = "";
	if ( $line =~ m!//.*! ) {
		$comment = $&;
		$line =~ s!//.*!!;
	}
	my $start = "";
	while (length($line) >= 79 - length($comment)) {
		$_ = $line;
		# this makes sure that the comment has something in front of it
		$len = 75 - length($comment);
		m!^(.{0,$len}) (.*)$!;
		$start = $start.$1."\n";
		$line = $indent."    ".$2;
	}
	print $start.$line.$comment."\n";
}
