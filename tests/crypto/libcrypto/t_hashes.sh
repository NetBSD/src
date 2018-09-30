# $NetBSD: t_hashes.sh,v 1.3.2.1 2018/09/30 01:45:58 pgoyette Exp $
#
# Copyright (c) 2008, 2009, 2010 The NetBSD Foundation, Inc.
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

atf_test_case hmac
hmac_head()
{
	atf_set "descr" "Checks HMAC message authentication code"
}
hmac_body()
{
	atf_check -o ignore -e ignore "$(atf_get_srcdir)/h_hmactest"
}

atf_test_case md2
md2_head()
{
	atf_set "descr" "Checks MD2 digest"
}
md2_body()
{
	atf_check -o ignore -e ignore "$(atf_get_srcdir)/h_md2test"
}

atf_test_case mdc2
mdc2_head()
{
	atf_set "descr" "Checks MDC2 digest"
}
mdc2_body()
{
	atf_check -o ignore -e ignore "$(atf_get_srcdir)/h_mdc2test"
}

atf_init_test_cases()
{
	atf_add_test_case hmac
	atf_add_test_case md2
	atf_add_test_case mdc2
}
