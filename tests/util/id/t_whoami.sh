# $NetBSD: t_whoami.sh,v 1.2.4.2 2008/01/09 01:59:39 matt Exp $
#
# Copyright (c) 2007 The NetBSD Foundation, Inc.
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
#        This product includes software developed by the NetBSD
#        Foundation, Inc. and its contributors.
# 4. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
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

run_whoami() {
	[ -f ./whoami ] || ln -s $(atf_get_srcdir)/h_id ./whoami
	./whoami "${@}"
}

atf_test_case correct
correct_head() {
	atf_set "descr" "Checks that correct queries work"
}
correct_body() {
	echo "Checking with EUID=100"
	echo "test" >expout
	atf_check "run_whoami" 0 expout null

	echo "Checking with EUID=0"
	export LIBFAKE_EUID_ROOT=1
	echo "root" >expout
	atf_check "run_whoami" 0 expout null
}

atf_test_case syntax
syntax_head() {
	atf_set "descr" "Checks the command's syntax"
}
syntax_body() {
	# Give a user to the command.
	echo 'usage: whoami' >experr
	atf_check "run_whoami root" 1 null experr

	# Give an invalid flag but which is allowed by id (with which
	# whoami shares code) when using the -un options.
	echo 'usage: whoami' >experr
	atf_check "run_whoami -r" 1 null experr
}

atf_init_test_cases()
{
	atf_add_test_case correct
	atf_add_test_case syntax
}
