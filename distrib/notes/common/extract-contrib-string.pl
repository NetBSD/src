#!/usr/bin/env perl
#
# Copyright (c) 2004, 2008 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Hubert Feyrer <hubert@feyrer.de>.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

#
# Extract BSD-mandated copyright messages for NetBSD documentation
#
# Usage:
# 1) find /usr/src -type f -print \
#    | perl extract-contrib-string.pl
#    >x
#
# 2) merge text after "--------" in "x" into
#    src/distrib/notes/common/legal.common
#
# Options:
#
#     perl extract-contrib-string.pl [-d] [-h]
#
# where
#     -d  debug output
#     -h  html output


$ack_line1="[aA]ll( commercial)?( marketing or)? advertising materials mentioning( features)?";
$ack_line2="display the following( acknowledge?ment)?";
$ack_endline=
      '(\d\.\s*(Neither the name'
    .         '|The name of the company nor the name'	# Wasn't my idea
    .         '|The name of the author may not'
    .         '|The name of .* must not be used to endorse'
    .         '|The names? (of )?.* nor the names? of'
    .         '|The names? (of )?.* or any of it\'?s members'
    .         '|Redistributions of any form whatsoever'
    .         '|The names .*"OpenSSL Toolkit.*" and .*"OpenSSL Project.*" must not be used))'
    .'|(THIS SOFTWARE IS PROVIDED)'
    .'|(The word \'cryptographic\' can be left out if)'
    .'|(may be used to endorse)'
    .'|(@end cartouche)'
    .'|(Redistribution and use in source and binary forms)'
    .'|(may not be used to endorse)'
    .'|(\.IP 4)'
    .'|(ALLOW FREE USE OF)'
    .'|(materials provided with the distribution)'
    .'|(@InsertRedistribution@)';

$known_bad_clause_3_wording=
      'usr.bin/lex/.*'				# UCB
    .'|usr.sbin/hilinfo/hilinfo.c'	   	# CSS @ Utah
    ;	

sub warning {
    local($fn,$msg) = @_;
    print "XXX $fn line $.: $msg\n"
}


if ($ARGV[0] =~ /-[dD]/) {
    $debug=1;
    shift(@ARGV);
}
if ($ARGV[0] =~ /-[hH]/) {
    $html=1;
    shift(@ARGV);
}


file:
while(<>) {
    chomp();
    $fn=$_;
    
    open(F, "$fn") || die "cannot read $fn: $!\n";

  line:
    while(<F>) {
	if (0 and /$ack_line2/i){
	    print "?> $_" if $debug;
	    
	    if ($fn !~ m,$known_bad_clause_3_wording,) {
		warning($fn, "clause 3 start not caught");
	    }
	    last line;
	}
	
	print "0> $_" if $debug;

	if (/$ack_line1/i
	    or (/$ack_line2/ and $fn =~ m,$known_bad_clause_3_wording,)) {
	    
	    print "1> $_" if $debug;

	    $_=<F>
		unless $fn =~ m,$known_bad_clause_3_wording,;
	    if (/$ack_line2/i or $fn =~ m,$known_bad_clause_3_wording,){
		
		print "2> $_" if $debug;
		
		$msg="";
		$cnt=0;
		$_=<F>;
		while(!/$ack_endline/i) {
		    
		    print "C> $_" if $debug;

		    $msg .= $_;
		    $cnt++;
		    $_ = <F>;
		    if ($cnt > 10) {
			warning($fn,"loooong copyright?");
			last line;
		    }
		}

		print "E> $_" if $debug;
		
		# post-process
		$msg =~ s/^\@c\s*//g;			# texinfo
		$msg =~ s/\n\@c\s*/\n/g;		# texinfo
		$msg =~ s/^REM\s*//g;			# BASIC?!?
		$msg =~ s/\nREM\s*/\n/g;		# BASIC?!?
		$msg =~ s/^dnl\s*//g;			# m4
		$msg =~ s/\dnl\s*/\n/g;			# m4
		$msg =~ s/^\.\\"\s*//g;			# *roff
		$msg =~ s/\n\.\\"\s*/\n/g;		# *roff
		$msg =~ s/^[#\\\|";]*\s*//g;		# sh etc.
		$msg =~ s/\n[#\\\|";]\s*/\n/g;		# sh etc.
		$msg =~ s/^[ 	*]*//g;      		# C
		$msg =~ s/\n[ 	*]*/\n/g;    		# C

		# C++/C99
		while ($msg =~ /^\s*\/\/\s*/) {
			$msg =~ s/^\s*\/\/\s*//o;
		}
		while ($msg =~ /\ns*\/\/\s*$/) {
			$msg =~ s/\ns*\/\/\s*$//o;
		}
		$msg =~ s/\ns*\/\/\s*/ /g;

		$msg =~ s/\@cartouche\n//;              # texinfo

		$msg =~ s///g;
		$msg =~ s/\s*\n/\n/g;
		$msg =~ s/^\s*//;
		$msg =~ s/\\\@/\@/g;
		$msg =~ s/\n\n/\n/g;
	        $msg =~ s/^\s*"//;
	        $msg =~ s/"\s*$//;
	        $msg =~ s/^\s*``//;
	        $msg =~ s/''\s*$//;
		while ($msg =~ /[\n\s]+$/) {
			$msg =~ s/[\n\s]+$//o;
		}

		# Split up into separate paragraphs
		#
		$msgs=$msg;
		$msgs=~s/(This (software|product))/|$1/g;
		$msgs=~s,^\|,,;
	      msg:
		foreach $msg (split(/\|/, $msgs)) {
		    if (!$html) {
			print ".\\\" File $fn:\n";
			print "$msg";
			print "\n\n";
		    }
		    
		    # Figure out if there's a version w/ or w/o trailing dot
		    # 
		    if ($msg =~ /\.\n$/) {
			# check if there's a version of the same msg
			# w/ a trailing dot
			$msg2=$msg;
			$msg2=~s,\.\n$,\n,;
			if ($copyrights{"$msg2"}) {
			    # already there - skip
			    print "already there, w/o dot - skipping!\n"
				if $debug;
			    next msg;
			}
			
			# ... maybe with other case?
			$lc_msg2=lc($msg2);
			if ($lc_copyrights{$lc_msg2}) {
			    print "already there, in different case - skipping\n"
				if $debug;
			    next msg;
			}
		    } else {
			# check if there's a version of the same msg
			# w/o the trailing dot
			$msg2=$msg;
			chomp($msg2);
			$msg2.=".\n";
			if ($copyrights{"$msg2"}) {
			    # already there - skip
			    print "already there, w/ dot - skipping!\n"
				if $debug;
			    next msg;
			}
			
			# ... maybe with other case?
			$lc_msg2=lc($msg2);
			if ($lc_copyrights{$lc_msg2}) {
			    print "already there, in different case - skipping\n"
				if $debug;
			    next msg;
			}
		    }

		    $copyrights{$msg} = 1;
		    $lc_copyrights{$lc_msg} = 1;
		}		 

	    } else {
		print "?> $_" if $debug;

                if ($fn !~ m,$known_bad_clause_3_wording,) {
		    warning($fn, "bad clause 3?");
                }
		last line;
	    }
	}
    }
    close(F);
}


if ($html) {
    print "<ul>\n";
    foreach $msg (sort keys %copyrights) {
	print "<li>$msg</li>\n";
    }
    print "</ul>\n";
} else {
    print "------------------------------------------------------------\n";

    $firsttime=1;
    foreach $msg (sort keys %copyrights) {
	if ($firsttime) {
	    $firsttime=0;
	} else {
	    print ".It\n";
	}
	print "$msg\n";
    }
}
