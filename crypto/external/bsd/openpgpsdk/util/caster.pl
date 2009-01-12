#!/usr/bin/perl -w

use strict;
use warnings;

use Carp;

if($#ARGV != 1) {
    print STDERR "$0 <infile> <outfile>\n";
    exit 1;
}

my $infile=shift;
my $outfile=shift;

open(I,$infile) || croak "$infile: $!";
open(O,">$outfile") || croak "$outfile: $!";

print O "/* Generated from $infile by $0, do not edit. */\n\n";

print O "#include \"types.h\"\n";

my $lineno=0;
while(my $line=<I>) {
    chomp $line;
    ++$lineno;

    next if $line eq '';
    next if $line =~ /^\/\//;

    my($to,$from)=$line =~ /^\s*(.+?)\s*->\s*(.+?)\s*$/;

    my($ttype,$tname,$targs)=parse($to);
    my($ftype,$fname,$fargs)=parse($from);
    print O "\n";
#    print O "#line $lineno \"$infile\"\n";
    print O "/* (line $lineno) $line */\n";
    print O "$from;\n";

    print O "#define $tname(";
    print_list($targs,1,1);
    print O ") $fname(";
    print_checked_list($targs, $fargs);
    print O ")\n";

    print O "typedef $ttype ${tname}_t(";
    print_list($targs,0,1);
    print O ");\n";
}

sub parse {
    my $func=shift;

    my($p1,$args)=$func =~ /^(.+?)\s*\((.+)\)$/;
    my($ftype,$fname)=parse_decl($p1);
#    print "func=$func p1=$p1 ftype=$ftype fname=$fname args=$args\n";
    my @args=split /,/,$args;
    my @alist;
    foreach my $arg (@args) {
	my($atype,$aname)=parse_decl($arg);
#	print "atype=$atype aname=$aname\n";
	push @alist,[$atype,$aname];
    }
    return ($ftype,$fname,\@alist);
}

sub parse_decl {
    my $decl=reverse shift;

    my($n,$t)=$decl =~ /^([A-Za-z0-9_]+)(.+)$/;
#    print "decl=$decl n=$n t=$t\n";

    $n=reverse $n;
    $t=reverse $t;
    return ($t,$n);
}

sub print_list {
    my $list=shift;
    my $member=shift;
    my $skip_extra=shift;

    my $first=1;
    foreach my $arg (@$list) {
	next if $skip_extra && $arg->[0] =~ /^\+/;
	print O ',' if !$first;
	my $str=$arg->[$member];
	$str =~ s/^\+//;
	print O $str;
	$first=undef;
    }
}

sub print_checked_list {
    my $types=shift;
    my $args=shift;

    for(my $n=0 ; $n <= $#$types ; ++$n) {
	print O ',' if $n;
	my $type=$types->[$n]->[0];
	$type =~ s/^\+//;
	my $arg=$args->[$n]->[1];
	$arg =~ s/^\+//;
	print O "CHECKED_INSTANCE_OF($type, $arg)";
    }
}
