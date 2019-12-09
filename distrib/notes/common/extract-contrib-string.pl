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
# 1) find src xsrc -type f -print \
#    | perl extract-contrib-string.pl
#    >x
#
# 2) merge text after "--------" in "x" into
#    src/distrib/notes/common/legal.common
#
# Options:
#
#     perl extract-contrib-string.pl [-d] [-h] [-x] [-?]
#
# where
#     -d  debug output
#     -h  html output
#     -x  xml/docbook output
#     -?  display help/usage message


$ack_line1='([aA]ll( commercial)?( marketing or)? advertising materials mentioning( features)?'
    .      '|\d\. Redistributions of any form whatsoever)';
$ack_line2='(display the( following)?( acknowledge?ment)?|acknowledge?ment:$)';
$ack_endline=
      '(\d\.\s*(Neither the name'
    .         '|The name of the company nor the name'	# Wasn't my idea
    .         '|The name of the author may not'
    .         '|The name of .* must not be used to endorse'
    .         '|The names? (of )?.* nor the names? of'
    .         '|The names? (of )?.* or any of it\'?s members'
    .         '|Redistributions of any form whatsoever'
    .         '|The names .*"OpenSSL Toolkit.*" and .*"OpenSSL Project.*" must not be used'
    .         "|Urbana-Champaign Independent Media Center's name"
    . '))'
    .'|(^Neither the name)'
    .'|(THIS SOFTWARE IS PROVIDED)'
    .'|(ALL WARRANTIES WITH REGARD)'
    .'|(The word \'cryptographic\' can be left out if)'
    .'|(may be used to endorse)'
    .'|(@end cartouche)'
    .'|(</para>)'
    .'|(Redistribution and use in source and binary forms)'
    .'|(may not be used to endorse)'
    .'|(\.IP 4)'
    .'|(ALLOW FREE USE OF)'
    .'|(materials provided with the distribution)'
    .'|(@InsertRedistribution@)';

$known_bad_clause_3_wording=
      'usr.bin/lex/.*'				# UCB
    .'|dist/bind/contrib/nslint-2.1a3/lbl/.*'	#
    .'|usr.sbin/traceroute/ifaddrlist.h'	#
    .'|usr.sbin/traceroute/traceroute.c'	#
    .'|usr.sbin/hilinfo/hilinfo.c'	   	# CSS @ Utah
    ;	

sub warning {
    local($fn,$msg) = @_;
    print "XXX $fn line $.: $msg\n"
}

while ($#ARGV >= 0) {
    $debug=1 if ($ARGV[0] =~ /-d/i);
    $html=1  if ($ARGV[0] =~ /-h/i);
    $xml=1  if ($ARGV[0] =~ /-x/i);
    $usage=1  if ($ARGV[0] =~ /-\?/);
    shift(@ARGV);
}

if ($usage) {
    print "usage: find /usr/src -type f -print |\n" .
	" perl extract-contrib-string.pl [-h] [-x] [-?] [-d]\n" .
	"   where\n" .
	"    -h   output html\n" .
	"    -x   output xml/docbook\n" .
	"    -d   debug\n" .
	"    -?   display this help message\n";
    exit(0);
}

$comments = !$html && !$xml;

file:
while(<>) {
    chomp();
    $fn=$_;
    
    open(F, "$fn") || die "cannot read $fn: $!\n";

  line:
    while(<F>) {
	if (0 and /$ack_line2/in){
	    print "?> $_" if $debug;
	    
	    if ($fn !~ m,$known_bad_clause_3_wording,) {
		warning($fn, "clause 3 start not caught");
	    }
	    last line;
	}
	
	print "0> $_" if $debug;

	# special case perl script generating a license (openssl's
	# mkerr.pl) - ignore the quoted license, there is another one
	# inside:
	if (/^\"\s\*.*$ack_line1.*\\n\"\,/n) {
		while(!/$ack_endline/in) {
		    print "S> $_" if $debug;
		    $_ = <F>;
		}
	}

	if (/$ack_line1/in
	    or (/$ack_line2/n and $fn =~ m,$known_bad_clause_3_wording,)) {
	    
	    print "1> $_" if $debug;

	    $_=<F>
		unless $fn =~ m,$known_bad_clause_3_wording,;
	    if (/$ack_line2/in or $fn =~ m,$known_bad_clause_3_wording,){
		
		print "2> $_" if $debug;
		
		$msg="";

		if ($fn =~ m,$known_bad_clause_3_wording, and /``/) {
		    $msg = $_;
		}
		elsif (/:\s+This product/) {
		    # src/sys/lib/libkern/rngtest.c - bad clause 3 wording
		    # that is not like others, so special case it here
		    $msg = $_;
		    $msg =~ s/^.*:\s+(This product.*)$/$1/;
		}

		$cnt=0;
		$_=<F>;
		while(!/$ack_endline/in) {
		    
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

		if ($fn =~ m,$known_bad_clause_3_wording,) {
			while ($msg !~ /^.*``.*\n/) {
				last if (!$msg);
				$msg =~ s/^.*\n//o;
			}
			$msg =~ s/^.*``//o;
			$msg =~ s/\n.*``//o;
			$msg =~ s/''.*$//o;
		}

		# XXX: pcap &c - add to known_bad_clause_3_wording but
		# that code seems to have problems.  Easier to add a
		# hack here, shouldn't affect good clause 3.
		$msg =~ s/''\s+Neither the name.*$//;

		# *roff
		while ($msg =~ /^\.\\"\s*/) {
			$msg =~ s/^\.\\"\s*//o;
		}
		while ($msg =~ /\n\.\\"\s*/) {
			$msg =~ s/\n\.\\"\s*/\n/o;
		}
		$msg =~ s/\n\.\\"\s*$/\n/g;

		# C++/C99
		while ($msg =~ /^\s*\/\/\s*/) {
			$msg =~ s/^\s*\/\/\s*//o;
		}
		while ($msg =~ /\n\s*\/\/\s*$/) {
			$msg =~ s/\n\s*\/\/\s*$//o;
		}
		$msg =~ s/\n\s*\/\/\s*/\n/g;

		# C
		while ($msg =~ /^\s*\*\s*/) {
			$msg =~ s/^\s*\*\s*//o;
		}
		while ($msg =~ /\n\s*\*\s*$/) {
			$msg =~ s/\n\s*\*\s*$//o;
		}
		$msg =~ s/\n\s*\*\s*/\n/g;

		# texinfo @c
		while ($msg =~ /^\s*\@c\s+/) {
			$msg =~ s/^\s*\@c\s+//o;
		}
		while ($msg =~ /\n\s*\@c\s+$/) {
			$msg =~ s/\n\s*\@c\s+$//o;
		}
		$msg =~ s/\n\s*\@c\s+/\n/g;

		$msg =~ s/^REM\s*//g;			# BASIC?!?
		$msg =~ s/\nREM\s*/\n/g;		# BASIC?!?
		$msg =~ s/^dnl\s*//g;			# m4
		$msg =~ s/\ndnl\s*/\n/g;		# m4
		$msg =~ s/^\s+-\s+//g;			# seen in docbook files
		$msg =~ s/\n\s+-\s+/ /g;		#
		$msg =~ s/^[#\\\|";]+\s*//g;		# sh etc.
		$msg =~ s/\n[#\\\|";]+\s*/\n/g;		# sh etc.
		$msg =~ s/^[ 	*]*//g;      		# C
		$msg =~ s/\n[ 	*]*/\n/g;    		# C

		$msg =~ s/\@cartouche\n//;              # texinfo

		$msg =~ s///g;
		$msg =~ s/\s*\n/\n/g;
		$msg =~ s/^\s*//;
		$msg =~ s/\\\@/\@/g;
		$msg =~ s/\n\n/\n/g;
	        $msg =~ s/^\s*``//;
	        $msg =~ s/''\s*$//;
		$msg =~ s/^\"//o;
		$msg =~ s/\"$//o;
		$msg =~ s/\"\.$/./o;

		# Fix ISO-646-SE spelling of Lule\(oa
		$msg =~ s/Lule\}/Lule\\(oa/g;

		# Collapse multiple spaces between words.  There are a
		# few entries with "by__Name" that affects sorting.
		$msg =~ s/(\w)  +(\w)/$1 $2/g;

		# Split up into separate paragraphs
		#
		$msgs=$msg;
		$msgs=~s/(This (software|product))/|$1/g;
		$msgs=~s,^\|,,;
	      msg:
		foreach $msg (split(/\|/, $msgs)) {
		    while ($msg =~ /[\n\s]+$/) {
			$msg =~ s/[\n\s]+$//o;
		    }
		    next if ($msg eq "");
		    if ($comments) {
			print ".\\\" File $fn:\n";
			print "$msg";
			print "\n\n";
		    }

		    my $key = lc($msg);	# ignore difference in case
		    $key =~ s/\n/ /g;	# ignore difference in line breaks
		    $key =~ s/\.$//g;	# drop the final dot

		    # push organizations ("by the") to the end of the
		    # sorting order
		    $key =~ s/(developed by) the/$1 ~the/;

		    if (defined $copyrights{$key}) {
			if ($copyrights{$key} !~ /\.$/ && $msg =~ /\.$/) {
			    print "already there, without dot - overriding!\n"
				if 1 || $debug;
			}
			else {
			    next msg;
			}
		    }

		    $copyrights{$key} = $msg;
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
    foreach $key (sort keys %copyrights) {
	my $msg = $copyrights{$key};
	print "<li>$msg</li>\n";
    }
    print "</ul>\n";
} elsif ($xml) {
    foreach $key (sort keys %copyrights) {
	my $msg = $copyrights{$key};
	print "<listitem>$msg</listitem>\n";
    }
} else {
    print "------------------------------------------------------------\n";

    $firsttime=1;
    foreach $key (sort keys %copyrights) {
	my $msg = $copyrights{$key};
	if ($firsttime) {
	    $firsttime=0;
	} else {
	    print ".It\n";
	}
	print "$msg\n";
    }
}
