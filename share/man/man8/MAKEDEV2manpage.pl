#!/usr/pkg/bin/perl
#
#	$NetBSD: MAKEDEV2manpage.pl,v 1.1 1999/09/18 00:15:13 hubertf Exp $
#
# Copyright (c) 1999
#	Hubert Feyrer <hubert@feyrer.de>.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
#
###########################################################################
#
# Convert src/etc/etc.${ARCH}/MAKEDEV and
# src/share/man/man8/man8.${ARCH}/MAKEDEV.8.template to  
# src/share/man/man8/man8.${ARCH}/MAKEDEV.8, replacing
#  - @@@SPECIAL@@@ with all targets in the first section (all, std, ...)
#  - @@@DEVICES@@@ with the remaining targets
#

$_lastline = "";

###########################################################################
# return 1 logical device-line
sub read1line
{
    local($h, $l);

    if ($_lastline ne "") {
	$h = $_lastline;
    } else {
	$h = <MAKEDEV>;
    }
    
    # Skip empty lines 
    while ($h =~ /^#\s*$/) {
	   $h = <MAKEDEV>;
    }
    
    if ($h =~ /^#\s/) {
	if ($h =~ /^# /) {
	    # Not a device/other target
	    $_lastline = "";
	    $l = $h;
	} else {
	    # Continuation line (?)
	    chomp($l = $h);
	    $h = <MAKEDEV>;
	    while ($h =~ m/^#\t\s/) {
		$h =~ s/^#\t\s/ /;
		$l .= $h;
	        $h = <MAKEDEV>;
	    }
	    $_lastline = $h;
	}
    } else {
	$_lastline = "";
	$l = $h;
    }

    return $l;
}

###########################################################################
# handle @@@SPECIAL@@@
sub do_special
{
    print MANPAGE ".Bl -tag -width 01234567 -compact\n";

    while (<MAKEDEV>) {
	last if /^#.*Device.*Valid.*argument/;
    }

    $l=read1line();
    while($l =~ /^#\t/) {
	  $l =~ s/#\s+//;
	  
	  ($target, @line) = split(/\s+/, $l);
	  $l = join(" ", @line);

	  $l =~ s/"([^"]*)"/``$1''/g;		# "..." -> ``...''
	  $l =~ s/\s*(MAKEDEV(.local)?)\s*/\n.Pa $1\n/g;
          $l =~ s/^(.)/\u$1/; # uppercase first word
	  print MANPAGE ".It Ar $target\n";
	  print MANPAGE "$l\n";
	  
	  $l = read1line();
    }
    $_lastline = $l; # unread
    
    print MANPAGE ".El\n";
}

###########################################################################
# handle @@@DEVICES@@@
sub do_devices
{
    print MANPAGE ".Bl -tag -width 01\n";

    $l = read1line();
    do {
	$l =~ s/#\s+//;
	print MANPAGE ".It $l";     # Print section heading 

	$l = read1line();
	print MANPAGE ". Bl -tag -width 0123456789 -compact\n";
	while($l =~ /^#\t/) {
	      $l =~ s/#\s+//;
	      
	      ($target, @line) = split(/\s+/, $l);
	      $target =~ s/\*/#/;
	      $l = join(" ", @line);

	      $l =~ s/"([^"]*)"/``$1''/g;		# "..." -> ``...''
  	      $l =~ s/\s*(MAKEDEV(.local)?)\s*/\n.Pa $1\n/g;
              $l =~ s/^(.)/\u$1/; # uppercase first word

              $l =~ s/\(XXX[^)]*\)//;
              $l =~ s/\s*$//;

              # add manpage, if available
              ($page) = ( $target=~/([a-zA-Z]+)/ );
              $page = "fdc" if $page eq "fd";
              if ( -f "../man4/$page.4" or -f "../man4/man4.${arch}/$page.4" ) {
                  $l =~ s/\s+$//;
                  if ($l =~ /see/) {
                      # already a manpage there, e.g. scsictl(8)
                      $l .= ", ";
                  } else {
                      $l .= ", see ";
                  }
                  $l .= "$page(4)";
              }

              while ($l =~ s/\s*(\w+)\((\d)\)(.*)/\n.Xr \1 \2 \3/g){;}

	      print MANPAGE ". It Ar $target\n";
	      print MANPAGE "$l\n";
	  
	      $l = read1line();
        }
        print MANPAGE ". El\n";
    } while ($l =~ /# /);

    print MANPAGE ".El\n";
}

###########################################################################
sub doarch
{
    local($arch) = @_;

    return "no MAKEDEV file found"
	if ! -f "../../../etc/etc.${arch}/MAKEDEV";
    return "no man8.${arch}/MAKEDEV.8.template"
	if ! -f "man8.${arch}/MAKEDEV.8.template";

    rename("man8.${arch}/MAKEDEV.8", "man8.${arch}/MAKEDEV.8.old");

    open(MANPAGE, ">man8.${arch}/MAKEDEV.8")	or die;
    print MANPAGE ".\\\" *** ------------------------------------------------------------------\n";
    print MANPAGE ".\\\" *** This file was generated automatically\n";
    print MANPAGE ".\\\" *** from src/etc/etc.${arch}/MAKEDEV and\n";
    print MANPAGE ".\\\" *** src/share/man/man8/man8.${arch}/MAKEDEV.8.template\n";
    print MANPAGE ".\\\" *** \n";
    print MANPAGE ".\\\" *** DO NOT EDIT - any changes will be lost!!!\n";
    print MANPAGE ".\\\" *** ------------------------------------------------------------------\n";
    print MANPAGE ".\\\"\n";

    open(MAKEDEV, "../../../etc/etc.${arch}/MAKEDEV") or die;
    
    open(TEMPLATE, "man8.${arch}/MAKEDEV.8.template")	or die;
    while(<TEMPLATE>) {
	if (/^\@\@\@.*\@\@\@$/) {
	    print MANPAGE ".\\\" $_";
	    if (/^\@\@\@SPECIAL\@\@\@$/) {
		do_special();
	    } elsif (/^\@\@\@DEVICES\@\@\@$/) {
		do_devices();
	    }
	} else {
	    print MANPAGE "$_";
	}
    }
    close(TEMPLATE);
    close(MANPAGE);

    $rc=system("cmp >/dev/null 2>&1 'man8.${arch}/MAKEDEV.8.old' 'man8.${arch}/MAKEDEV.8'");
    unlink("man8.${arch}/MAKEDEV.8.old");

    if ($rc) {
	return "updated";
    } else {
	return "unchanged";
    }
}

###########################################################################
###########################################################################
###
###   M   A   I   N
### 
###########################################################################
###########################################################################
# cd /usr/src/share/man/man8
chomp($pwd=`pwd`);
die "Run this in .../src/share/man/man8 !\n" 
    if ($pwd !~ m:share/man/man8: );

# Determine available archs by looking for man8.* 
opendir(D, ".") || die;
while ($d=readdir(D)) {
    if ($d =~ /man8\.(.*)$/) {
	push(@archs, $1);
    }
}
closedir(d);

#DEBUG ONLY# @archs=("alpha");

# Handle each arch's manpage
foreach $arch ( sort @archs ) {
    $rc = doarch($arch);
    print "$arch:\t $rc\n";
}
