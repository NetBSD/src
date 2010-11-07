# $NetBSD: t_awk.sh,v 1.3 2010/11/07 17:51:22 jmmv Exp $
#
# Copyright (c) 2008, 2009 The NetBSD Foundation, Inc.
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

h_check()
{
	for sfx in in out awk; do
		cp -r $(atf_get_srcdir)/d_$1.$sfx .
	done
	atf_check -o file:d_$1.out -x "awk -f d_$1.awk < d_$1.in"
}

atf_test_case big_regexp
big_regexp_head()
{
	atf_set "descr" "Checks matching long regular expressions (PR/33392)"
}
big_regexp_body()
{
	h_check big_regexp
}

atf_test_case end
end_head()
{
	atf_set "descr" "Checks that the last line of the input" \
	                "is available under END pattern (PR/29659)"
}
end_body()
{
	h_check end1
	h_check end2
}

atf_test_case string1
string1_head()
{
	atf_set "descr" "Checks escaping newlines in string literals"
}
string1_body()
{
	for sfx in out awk; do
		cp -r $(atf_get_srcdir)/d_string1.$sfx .
	done
	atf_check -o file:d_string1.out awk -f d_string1.awk
}

atf_test_case multibyte
multibyte_head()
{
	atf_set "descr" "Checks multibyte charsets support" \
	                "in tolower and toupper (PR/36394)"
}
multibyte_body()
{
	export LANG=en_US.UTF-8

	h_check tolower
	h_check toupper
}

atf_init_test_cases()
{
	atf_add_test_case big_regexp
	atf_add_test_case end
	atf_add_test_case string1
	atf_add_test_case multibyte
}
