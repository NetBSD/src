#!/usr/bin/perl -w

use strict;
use Crypt::OpenSSL::RSA;
use Getopt::Std;
use MIME::Base64;
use Net::DNS;
use Net::DNS::SEC;

my %option;
getopts('a:e:i:l:p:hk',\%option);

die "usage: keyconv.pl [-a alg] [-k (to indicate KSK)] -e engine -l label [-p (path to store key)] -i filename domainname\n" if $option{h} || (not defined $option{i}) || (not defined $option{e}) || (not defined $option{l});

# The default path is local.
$option{p} || ($option{p}="./");

# The default algorithm is 5.
$option{a} || ($option{a}=5);

$option{k} || ($option{k}=0);

# The algorithm is either 5 or 133.
$option{a}==5 || $option{a}==133 || die "algorithm must be 5 or 133\n";

# standard flags (value is 256) plus optionally the KSK flag.
my $flags=(256 + $option{k});

open(PFILE, $option{i});
   my @fc = <PFILE>;
close(PFILE);

my $rsa = Crypt::OpenSSL::RSA->new_public_key(join "", @fc);

my ($m,$e)= $rsa->get_key_parameters;

(my $l=pack("Cn",0,length($e->to_bin))) =~ s/^\000{2}//;

my $rrkey=$l.$e->to_bin.$m->to_bin;
my $keystr = $ARGV[0]. ". IN DNSKEY $flags 3 $option{a} ".encode_base64($rrkey,"");
my $keyrr = Net::DNS::RR->new($keystr);

open(PFILE, "> $option{p}/K".$ARGV[0].".+".sprintf("%03d",$option{a})."+".$keyrr->keytag.".key");
print PFILE $ARGV[0], ". IN DNSKEY $flags 3 $option{a} ",encode_base64($rrkey,"")."\n";
close(PFILE);

open(PFILE, "> $option{p}/K".$ARGV[0].".+".sprintf("%03d",$option{a})."+".$keyrr->keytag.".private");
print PFILE "Private-key-format: v1.2\n";
print PFILE "Algorithm: ", $option{a}, " (RSASHA1)\n";
print PFILE "Modulus: ".encode_base64($m->to_bin,"")."\n";
print PFILE "PublicExponent: ".encode_base64($e->to_bin,"")."\n";
my $engine="";
$engine=encode_base64($option{e}."\0","");
print PFILE "Engine: ", $engine, "\n";
my $label="";
$option{k}==0 && ($label=encode_base64($option{e}.":".$option{l}."\0",""));
$option{k}!=0 && ($label=encode_base64($option{e}.":".$option{l}."\0",""));
print PFILE "Label: ", $label, "\n";
close(PFILE);

print $keyrr->keytag;
