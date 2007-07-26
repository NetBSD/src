#!/usr/bin/env perl
#
# Copyright (c) 2004 Hubert Feyrer <hubert@feyrer.de>
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#          This product includes software developed by Hubert Feyrer
#          for the NetBSD Project.
# 4. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# Usage:
# 1) lynx -dump http://www.netbsd.org/people/port-maintainers.html \
#    | perl list-portmasters.pl
#    >x
# 2) Sort: sort +1 x >xx ; mv xx x
# 3) merge "x" into src/distrib/notes/common/main's "portmasters" section
#
# Note:  If the *second* portmaster of a port has a link as part of the
#        Name field, it will cause this script to error out.

while(<>) {
    chomp;
    last if /Port.*Name.*E-mail.*Address/;
}

while(<>) {
    chomp;
    s/^\s*//;
    s/\s*$//;
    @a = split(/\s+/);

    $port=shift(@a);
    $email=pop(@a);
    $name="@a";

    if ($port && $port !~ /\[\d+\]/) {
	# Port name is wrong - probably because of dual portmasters.
	# Fix it up.
	$name = "$port $name";

	$port = $last_port;
    }

    # valid data is now in $last_*,
    # $* is saved for possible further processing
    
    $last_port=~s,\s*\[\d+\],,;
    $last_name=~s,\s*\[\d+\],,;
    $last_email=~s,\s*\[\d+\],,;
    $last_email=~s,\<,,;
    $last_email=~s,\>,,;

    $last_name=~s,ø,\\(/o,g;		# Søren => S\(/oren
    
    # output
    print ".It ";
    printf("Ta %-30s", $last_name);
    print " ";
    printf("Ta Mt %20s", $last_email);
    print " ";
    printf("Ta Sy %s", $last_port);
    print "\n";

    $last_port=$port;
    $last_email=$email;
    $last_name=$name;

    last if $name=~/______________/;
}
