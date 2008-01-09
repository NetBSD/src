# $NetBSD: t_expand.sh,v 1.1.4.2 2008/01/09 01:59:43 matt Exp $
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

#
# This file tests the functions in expand.c.
#

atf_test_case dollar_at
dollar_at_head() {
	atf_set "descr" "Somewhere between 2.0.2 and 3.0 the expansion" \
	                "of the \$@ variable had been broken.  Check for" \
			"this behavior."
}
dollar_at_body() {
	# This one should work everywhere.
	got=`echo "" "" | sed 's,$,EOL,'`
	atf_check_equal ' EOL' '$got'

	# This code triggered the bug.
	set -- "" ""
	got=`echo "$@" | sed 's,$,EOL,'`
	atf_check_equal ' EOL' '$got'

	set -- -
	shift
	n_arg() { echo $#; }
	n_args=`n_arg "$@"`
	atf_check_equal '0' '$n_args'
}

atf_test_case strip
strip_head() {
	atf_set "descr" "Checks that the %% operator works and strips" \
	                "the contents of a variable from the given point" \
			"to the end"
}
strip_body() {
	line='#define bindir "/usr/bin" /* comment */'
	stripped='#define bindir "/usr/bin" '
	atf_check_equal '$stripped' '${line%%/\**}'
}

atf_test_case arithmetic
arithmetic_head() {
	atf_set "descr" "POSIX requires shell arithmetic to use signed" \
	                "long or a wider type.  We use intmax_t, so at" \
			"least 64 bits should be available.  Make sure" \
			"this is true."
}
arithmetic_body() {
	atf_check_equal '3' '$((1 + 2))'
	atf_check_equal '2147483647' '$((0x7fffffff))'
	atf_check_equal '9223372036854775807' '$(((1 << 63) - 1))'
}

atf_init_test_cases() {
	atf_add_test_case dollar_at
	atf_add_test_case strip
	atf_add_test_case arithmetic
}
