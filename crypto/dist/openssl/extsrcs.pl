#!/usr/local/bin/perl
#
#	$NetBSD: extsrcs.pl,v 1.1.1.1 2000/06/14 22:44:19 thorpej Exp $

# NAME:
#	extsrcs - extract SRCS from Makefiles
#
# SYNOPSIS:
#	find . -name Makefile -print | \\
#		extsrcs.pl [-D "dist"][-S "strip"][-d "dir"] 
#
# DESCRIPTION:
#	This script facilitates importing SSLeay into a BSD build tree
#	by extracting the object file names (converted to .c's) from
#	the LIBOBJ setting.
#
#	For each subdir/Makefile, a subdir.inc file is created in
#	"dir".  Within the .inc file we add a .PATH line pointing at
#	"dist"/"subdir" after stripping "strip" from the front of
#	"subdir".   For example:
#
#.nf
#
#	echo /usr/local/src/SSLeay-0.9.0b/crypto/rsa/Makefile |\\
#	extsrcs.pl -S /usr/local/src/SSLeay-0.9.0b -d /tmp/ssleay
#.fi
#   
#	will print .include ``rsa.inc'' and create /tmp/ssleay/rsa.inc
#	containing:
#.nf
#   
#	\\.PATH:  ${SSLSRC}/crypto/rsa
#
#	CPPFLAGS+=  -I${SSLSRC}/crypto/rsa
#
#	SRCS+= rsa_eay.c rsa_gen.c rsa_lib.c rsa_sign.c rsa_saos.c rsa_err.c \
#		rsa_pk1.c rsa_ssl.c rsa_none.c
#.fi
#
#	It knows a little about some of the special variables that
#	the SSLeay Makefiles use to pick some objects and may need
#	some tweaking for each new version of SSLeay (or OpenSSL), but
#	it does handle the bulk of the task pretty well.
#
# AUTHOR:
#	Simon J. Gerraty <sjg@quick.com.au>

# RCSid:
#	from: Id: extsrcs.pl,v 1.3 1999/07/16 13:48:35 sjg Exp $
#
#	@(#)Copyright (c) 1998 QUICK.COM.AU. All rights reserved.
#	
#	This file is provided in the hope that it will
#	be of use.  There is absolutely NO WARRANTY.
#	Permission to use, modify and distribute this source code 
#	is granted subject to the following conditions.
#	1/ that the above copyright notice and this notice 
#	are preserved in all copies and that due credit be given 
#	to the author.	
#	2/ that any changes to this code are clearly commented 
#	as such so that the author does not get blamed for bugs 
#	other than his own.
#	3/ If there is an accompanying License file its provisions
#	must also be met.
#	
#	Please send copies of changes and bug-fixes to:
#	sjg@quick.com.au
#


require 'getopts.pl';
use File::Basename;

$opt_D = '${OPENSSLSRC}';
$opt_S = '.*openssl';
$opt_d = '.';

&Getopts('D:d:S:');

$ldir = '';

@all_args = ('RC4_ENC', 'RC5_ENC', 'DES_ENC', 'CAST_ENC', 'BF_ENC', 'BN_ASM');

$codestr2 = 'sub {' . "\n";
$codestr2 .= '  local($line) = @_;' . "\n";
$codestr = $codestr2;
for $arg (@all_args) {
  $codestr .= '  if ($line =~ m/^\s*' . $arg . '\s*=\s*(.*)/) {' . "\n";
  #$codestr .= '    print "found ' . $arg . '\n";' . "\n";
  $codestr .= '    $' . $arg . ' = $line;' . "\n";
  $codestr .= '    $' . $arg . ' =~ s/.*=//;' . "\n";
  $codestr .= '    $' . $arg . ' =~ s/\s/ /g;' . "\n";
  $codestr .= '  };' . "\n";
  $codestr2 .= '  $line =~ s/\$[{(]' . $arg . '[)}]/$' . $arg . '/g;' . "\n";
}
#$codestr .= '  print "returning $line";' . "\n";
$codestr .= '  return $line;' . "\n";
$codestr2 .= '  return $line;' . "\n";
$codestr .= '};' . "\n";
$codestr2 .= '};' . "\n";

#print "one: $codestr\n";
#exit;
$code = eval $codestr;

#print "two: $codestr2\n";
#exit;
$code2 = eval $codestr2;

while (<>) {
  chop;

  &ext($_);
}

sub ext {
  ($Makefile) = @_;
  
  $dir = &dirname($Makefile);

  if ($dir =~ m,/,) {
    $dir =~ s,^$opt_S/,,;
    $sub = &basename($dir);
#    $dir = &basename($dir);
  }
  
  open(M, "< $Makefile") || die;

  while ($line = <M>) {
    next if ($line =~ m/^\s*\#/);
    
    last if ($line =~ m/LIBOBJ.*=/);
    if ($line =~ m/ERRC.*=/) {
      $ERRC = $line;
      $ERRC =~ s/.*=//;
      $ERRC =~ s/\s//g;
    }
    if ($line =~ m/BN_MULW.*=/) {
      $BN_MULW = $line;
      $BN_MULW =~ s/.*=//;
      $BN_MULW =~ s/\s//g;
    }
    $line = &$code($line);
  }
  if ($line =~ m/LIBOBJ/) {
    open(STDOUT, "> $opt_d/$sub.inc");
    print "#\t\$NetBSD\$\n#\n#\t\@(#) Copyright (c) 1995 Simon J. Gerraty\n#\n";
    print "#\tSRCS extracted from $Makefile\n#\n\n";
    print ".PATH:\t$opt_D/$dir\n\n";
    print "CPPFLAGS+=\t-I$opt_D/$dir\n\n";

    for (;;) {
      $line =~ s/LIBOBJ.*=/SRCS+=/;
      $line =~ s/\$[{(]ERRC[)}]/$ERRC/g;
      $line =~ s/\$[{(]BN_MULW[)}]/$BN_MULW/g;
      $line = &$code2($line);
      $line =~ s/\.o/.c/g;
      print $line;

      $line = <M>;
      next if ($line =~ m/^\s*\#/);
      last unless($line =~ m/\.o/);
    }
    print "\n\n";
    printf STDERR ".include \"$sub.inc\"\n";
  }
  close M;
}


  
