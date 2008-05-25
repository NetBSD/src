# $NetBSD: t_set_e.sh,v 1.3 2008/05/25 19:25:03 dholland Exp $
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

# the implementation of "sh" to test
: ${TEST_SH:="sh"}

nl='
'

failwith()
{
	case "$SH_FAILS" in
		"") SH_FAILS=`echo "$1"`;;
		*) SH_FAILS="$SH_FAILS"`echo; echo "$1"`;;
	esac
}

check()
{
	result=`$TEST_SH -c "$1" 2>/dev/null`
	if [ "$result" != "$2" ]; then
		MSG=`printf "%-32s %-8s  %s" "$3" "$result" "$2"`
		failwith "$MSG"
		failcount=`expr $failcount + 1`
	fi
	count=`expr $count + 1`
}

checksimple()
{
	check 'p() { echo -n $1$?; }; ((set -e; '"$1"'; p X); p Y)' "$2" "simple       $1"
	check 'p() { echo -n $1$?; }; t='"'"'((set -e; '"$1"'; p X); p Y)'"'"'; eval $t' "$2" "simple-eval  $1"

}

atf_test_case all
all_head() {
	atf_set "descr" "Tests that 'set -e' works correctly"
}
all_body() {
	count=0
	failcount=0

	checksimple 'exit 1' 'Y1'
	checksimple 'false' 'Y1'
	checksimple '(false)' 'Y1'
	checksimple 'false || false' 'Y1'
	checksimple 'false | true' 'X0Y0'
	checksimple 'true | false' 'Y1'
	checksimple '/nonexistent' 'Y127'
	checksimple 'f() { false; }; f' 'Y1'

	if [ "x$SH_FAILS" != x ]; then
	    echo "How          Expression          Result    Should be"
	    echo "$SH_FAILS"
	    atf_fail "$failcount of $count failed cases"
	else
	    atf_pass
	fi
}

atf_init_test_cases() {
	atf_add_test_case all
}
