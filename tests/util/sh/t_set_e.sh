# $NetBSD: t_set_e.sh,v 1.1.4.2 2008/01/09 01:59:45 matt Exp $
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

nl='
'

check1()
{
	result="`eval $1`"
	# Remove newlines
	oifs="$IFS"
	IFS="$nl"
	result="`echo $result`"
	IFS="$oifs"
	atf_check_equal "$2" '$result'
}

p()
{

	if [ $? = 0 ]
	then
		echo ${1}0
	else
		echo ${1}1
	fi
}

check()
{
	check1 "((set -e; $1; p X); p Y)" "$2"
}

atf_test_case all
all_head() {
	atf_set "descr" "Tests that 'set -e' works correctly"
}
all_body() {
	check 'exit 1' 'Y1'
	check 'false' 'Y1'
	check '(false)' 'Y1'
	check 'false || false' 'Y1'
	check 'false | true' 'X0 Y0'
	check 'true | false' 'Y1'
	check '/nonexistent' 'Y1'
	check 'f() { false; }; f' 'Y1'
}

atf_init_test_cases() {
	atf_add_test_case all
}
