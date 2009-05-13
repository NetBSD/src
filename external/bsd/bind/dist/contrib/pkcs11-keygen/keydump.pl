#!/usr/bin/perl -w

use strict;
use Getopt::Std;
use Crypt::OpenSSL::RSA;
use Net::DNS::SEC;

my %option;
getopts('k:p:o:h',\%option);

$option{h} || (not defined $option{k}) || (not defined $option{p}) || (not defined $option{o}) && die "usage: keydump.pl -k Kxxx.key -p Kxxx.priv -o pem\n";

my $rsa = Net::DNS::SEC::Private->new($option{p});

open(PFILE, "> $option{o}");
print PFILE $rsa->dump_rsa_private_der;
close(PFILE);

open(KFILE, "< $option{k}");
my @fc = <KFILE>;
close(KFILE);

my $keyrr = Net::DNS::RR->new(join "", @fc);

print $keyrr->flags;

