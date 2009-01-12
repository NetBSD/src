#!/usr/bin/perl -w

use strict;

use Cwd;
use Carp;

my %flags;
my @args;
my @args2;
my $ifile;
my $cc;
my $is_compile;

foreach my $arg (@ARGV) {
    if($arg =~ /^--cc=(.+)$/) {
	$cc=$1;
    } else {
	$flags{$arg}=1;
	push @args,$arg;
	if($arg =~ /^([^-].*)\.c/) {
	    $ifile="$1.i";
	    push @args2,$ifile;
	} else {
	    push @args2,$arg;
	}
	$is_compile=1 if $arg eq '-c';
    }
}

my $pwd=getcwd();

if($is_compile) {
    my $cmd="$cc -E ".join(' ',@args);
    print STDERR "$cmd\n";
#    system "$cmd > $ifile";
    open(F,"$cmd|") || croak "$cmd: $!";
    open(O,">$ifile") || croak "$ifile: $!";
    while(my $line=<F>) {
      if($line =~ /^\# (\d+) "([^\/<].+)"(.*)$/) {
	print O "# $1 \"$pwd/$2\"$3\n";
#	print O $line;
      } else {
	print O $line;
      }
    }
    close O;
    close F || croak "Errors in parse: $! $?";
    $cmd="$cc  ".join(' ',@args2);
    print "$cmd\n";
    system($cmd) == 0 || croak "Errors in compile: $?";
} else {
    my $cmd="$cc ".join(' ',@args);
    print STDERR "$cmd\n";
    system $cmd;
}
