# $NetBSD: t_config.awk,v 1.1.2.2 2009/05/13 19:19:36 jym Exp $
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

/^tc:/ {
	printf "atf_test_case %s\n", $3;
	printf "%s_head()\n", $3;
	print  "{"
	printf "	atf_set \"descr\" \"%s\"\n", $4;
	print  "}"
	printf "%s_body()\n", $3;
	print  "{"
	printf "	h_%s %s\n", $2, d_$3;
	print  "}"
	print  ""

	tcs[count++] = $3;

	next
}

/^tc_list/ {
	for (i = 0; i < count; i++) {
		printf("	atf_add_test_case %s\n", tcs[i]);
	}

	next
}

{ print }
