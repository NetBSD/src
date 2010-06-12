# $NetBSD: t_ipf.awk,v 1.3 2010/06/12 14:07:18 pooka Exp $
#
# Copyright (c) 2008 The NetBSD Foundation, Inc.
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

BEGIN { 
	FS = ":";
};

function maketc(name, type, desc, rest, skip)
{

	printf "atf_test_case %s\n", name;
	printf "%s_head()\n", name;
	print  "{"
	printf "	atf_set \"descr\" \"%s\"\n", desc;
	printf "	atf_set \"use.fs\" \"true\"\n", desc;
	print  "}"
	printf "%s_body()\n", name;
	print  "{"

	if (skip) {
		printf "	atf_skip \"test suspected to be broken\"\n\n"
	}
	printf "	h_%s %s %s", type, name, rest;
	printf "\n";

	print  "}"
	print  ""

	tcs[count++] = $2;
}

/^tc:/ {
	desc = ($4 in descs) ? descs[$4] : $4;
	rest = "\"" $5 "\""
	for (i = 6; i <= NF; ++i)
		rest = rest " \"" $i "\""

	maketc($2, $3, desc, rest, 0)

	next
}

/^tc_skip:/ {
	desc = ($4 in descs) ? descs[$4] : $4;
	rest = "\"" $5 "\""
	for (i = 6; i <= NF; ++i)
		rest = rest " \"" $i "\""

	maketc($2, $3, desc, rest, 1)

	next
}

/^tc_desc/ {
	descs[$2] = $3;

	next
}

/^tc_list/ {
	for (i = 0; i < count; i++) {
		printf("	atf_add_test_case %s\n", tcs[i]);
	}

	next
}

{ print }
