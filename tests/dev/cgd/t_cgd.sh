#	$NetBSD: t_cgd.sh,v 1.1 2010/11/11 22:38:47 pooka Exp $
#
# Copyright (c) 2010 The NetBSD Foundation, Inc.
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

atf_test_case basic
basic_head()
{

	atf_set "descr" "Tests that encrypt/decrypt works"
}

basic_body()
{

	d=$(atf_get_srcdir)
	atf_check -s exit:0 sh -c "echo 12345 | \
	    $d/h_img2cgd/h_img2cgd $d/h_img2cgd/cgd.conf write \
	    enc.img $d/h_img2cgd/cgd.conf"
	atf_check -s exit:0 sh -c "echo 12345 | \
	    $d/h_img2cgd/h_img2cgd $d/h_img2cgd/cgd.conf read \
	    enc.img clear.txt"
	atf_check -s exit:0 cmp clear.txt $d/h_img2cgd/cgd.conf
}

atf_test_case wrongpass
wrongpass_head()
{

	atf_set "descr" "Tests that wrong password does not give original " \
	    "plaintext"
}

wrongpass_body()
{

	d=$(atf_get_srcdir)
	atf_check -s exit:0 sh -c "echo 12345 | \
	    $d/h_img2cgd/h_img2cgd $d/h_img2cgd/cgd.conf write \
	    enc.img $d/h_img2cgd/cgd.conf"
	atf_check -s exit:0 sh -c "echo 54321 | \
	    $d/h_img2cgd/h_img2cgd $d/h_img2cgd/cgd.conf read \
	    enc.img clear.txt"
	atf_check -s not-exit:0 cmp -s clear.txt $d/h_img2cgd/cgd.conf
}

atf_init_test_cases()
{

	atf_add_test_case basic
	atf_add_test_case wrongpass
}
