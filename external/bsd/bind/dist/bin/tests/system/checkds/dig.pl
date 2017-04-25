#!/usr/bin/perl
#
# Copyright (C) 2014  Internet Systems Consortium, Inc. ("ISC")
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

# $Id: dig.pl,v 1.1.1.1.10.3 2017/04/25 20:53:30 snj Exp $

my $arg;
my $ext;
my $file;

foreach $arg (@ARGV) {
    if ($arg =~ /^\+/) {
        next;
    }
    if ($arg =~ /^-t/) {
        next;
    }
    if ($arg =~ /^ds$/i) {
        $ext = "ds";
        next;
    }
    if ($arg =~ /^dlv$/i) {
        $ext = "dlv";
        next;
    }
    if ($arg =~ /^dnskey$/i) {
        $ext = "dnskey";
        next;
    }
    $file = $arg;
    next;
}

open F, $file . "." . $ext . ".db" || die $!;
while (<F>) {
    print;
}
close F;
