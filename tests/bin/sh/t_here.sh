# $NetBSD: t_here.sh,v 1.4 2016/03/08 14:21:02 christos Exp $
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
: ${TEST_SH:="/bin/sh"}

nl='
'

check()
{
	fail=false
	TEMP_FILE=$( mktemp OUT.XXXXXX )

	# our local shell (ATF_SHELL) better do quoting correctly...
	# some of the tests expect us to expand $nl internally...
	CMD="nl='${nl}'; $1"

	rm -f trace.*
	result="$( ${TEST_SH} -c "${CMD}" 2>"${TEMP_FILE}" )"
	STATUS=$?

	if [ "${STATUS}" -ne "$3" ]; then
		echo >&2 "expected exit code $3, got ${STATUS}"

		# don't actually fail just because of wrong exit code
		# unless we either expected, or received "good"
		case "$3/${STATUS}" in
		(*/0|0/*) fail=true;;
		esac
	fi

	if [ "$3" -eq 0 ]; then
		if [ -s "${TEMP_FILE}" ]; then
			echo >&2 "Messages produced on stderr unexpected..."
			cat "${TEMP_FILE}" >&2
			fail=true
		fi
	else
		if ! [ -s "${TEMP_FILE}" ]; then
			echo >&2 "Expected messages on stderr, nothing produced"
			fail=true
		fi
	fi
	rm -f "${TEMP_FILE}"

	# Remove newlines (use local shell for this)
	oifs="$IFS"
	IFS="$nl"
	result="$(echo $result)"
	IFS="$oifs"
	if [ "$2" != "$result" ]
	then
		echo >&2 "Expected output '$2', received '$result'"
		fail=true
	fi

	$fail && atf_fail "test of '$1' failed"
	return 0
}

atf_test_case do_simple
do_simple_head() {
	atf_set "descr" "Basic tests for here documents"
}
do_simple_body() {
	y=x

	IFS=
	check 'x=`cat <<EOF'$nl'text'${nl}EOF$nl'`; echo $x' 'text' 0
	check 'x=`cat <<\EOF'$nl'text'${nl}EOF$nl'`; echo $x' 'text' 0

	check "y=${y};"'x=`cat <<EOF'$nl'te${y}t'${nl}EOF$nl'`; echo $x' \
			'text' 0
	check "y=${y};"'x=`cat <<\EOF'$nl'te${y}t'${nl}EOF$nl'`; echo $x'  \
			'te${y}t' 0
	check "y=${y};"'x=`cat <<"EOF"'$nl'te${y}t'${nl}EOF$nl'`; echo $x'  \
			'te${y}t' 0
	check "y=${y};"'x=`cat <<'"'EOF'"$nl'te${y}t'${nl}EOF$nl'`; echo $x'  \
			'te${y}t' 0

	# check that quotes in the here doc survive and cause no problems
	check "cat <<EOF${nl}te'xt${nl}EOF$nl" "te'xt" 0
	check "cat <<\EOF${nl}te'xt${nl}EOF$nl" "te'xt" 0
	check "cat <<'EOF'${nl}te'xt${nl}EOF$nl" "te'xt" 0
	check "cat <<EOF${nl}te\"xt${nl}EOF$nl" 'te"xt' 0
	check "cat <<\EOF${nl}te\"xt${nl}EOF$nl" 'te"xt' 0
	check "cat <<'EOF'${nl}te\"xt${nl}EOF$nl" 'te"xt' 0
	check "cat <<'EO'F${nl}te\"xt${nl}EOF$nl" 'te"xt' 0

	check "y=${y};"'x=`cat <<EOF'$nl'te'"'"'${y}t'${nl}EOF$nl'`; echo $x' \
			'te'"'"'xt' 0
	check "y=${y};"'x=`cat <<EOF'$nl'te'"''"'${y}t'${nl}EOF$nl'`; echo $x' \
			'te'"''"'xt' 0

	# note that the blocks of empty space in the following must
	# be entirely tab characters, no spaces.

	check 'x=`cat <<EOF'"$nl	text${nl}EOF$nl"'`; echo "$x"' \
			'	text' 0
	check 'x=`cat <<-EOF'"$nl	text${nl}EOF$nl"'`; echo $x' \
			'text' 0
	check 'x=`cat <<-EOF'"${nl}text${nl}	EOF$nl"'`; echo $x' \
			'text' 0
	check 'x=`cat <<-\EOF'"$nl	text${nl}	EOF$nl"'`; echo $x' \
			'text' 0
	check 'x=`cat <<- "EOF"'"$nl	text${nl}EOF$nl"'`; echo $x' \
			'text' 0
	check 'x=`cat <<- '"'EOF'${nl}text${nl}	EOF$nl"'`; echo $x' \
			'text' 0
}

atf_test_case incomplete
incomplete_head() {
	atf_set "descr" "Basic tests for incomplete here documents"
}
incomplete_body() {
	check 'cat <<EOF' '' 2
	check 'cat <<- EOF' '' 2
	check 'cat <<\EOF' '' 2
	check 'cat <<- \EOF' '' 2

	check 'cat <<EOF'"${nl}" '' 2
	check 'cat <<- EOF'"${nl}" '' 2
	check 'cat <<'"'EOF'${nl}" '' 2
	check 'cat <<- "EOF"'"${nl}" '' 2

	check 'cat << EOF'"${nl}${nl}" '' 2
	check 'cat <<-EOF'"${nl}${nl}" '' 2
	check 'cat << '"'EOF'${nl}${nl}" '' 2
	check 'cat <<-"EOF"'"${nl}${nl}" '' 2

	check 'cat << EOF'"${nl}"'line 1'"${nl}" '' 2
	check 'cat <<-EOF'"${nl}"'	line 1'"${nl}" '' 2
	check 'cat << EOF'"${nl}"'line 1'"${nl}"'	line 2'"${nl}" '' 2
	check 'cat <<-EOF'"${nl}"'	line 1'"${nl}"'line 2'"${nl}" '' 2

	check 'cat << EOF'"${nl}line 1${nl}${nl}line3${nl}${nl}5!${nl}" '' 2
}

atf_test_case multiple
multiple_head() {
	atf_set "descr" "Tests for multiple here documents for one cmd"
}
multiple_body() {
	check \
    "(cat ; cat <&3) <<EOF0 3<<EOF3${nl}STDIN${nl}EOF0${nl}-3-${nl}EOF3${nl}" \
		'STDIN -3-' 0

	check "(read line; echo \"\$line\"; cat <<EOF1; echo \"\$line\") <<EOF2
The File
EOF1
The Line
EOF2
"			'The Line The File The Line' 0

	check "(read line; echo \"\$line\"; cat <<EOF; echo \"\$line\") <<EOF
The File
EOF
The Line
EOF
"			'The Line The File The Line' 0

}

atf_test_case vicious
vicious_head() {
	atf_set "descr" "Tests for obscure and obnoxious uses of here docs"
}
vicious_body() {

	cat <<- \END_SCRIPT > script
		cat <<ONE && cat \
		<<TWO
		a
		ONE
		b
		TWO
	END_SCRIPT

	atf_check -s exit:0 -o inline:'a\nb\n' -e empty ${TEST_SH} script

	# This next one is causing discussion currently (late Feb 2016)
	# amongst stds writers & implementors.   Consequently we
	# will not check what it produces.   The eventual result
	# seems unlikely to be what we currently output, which
	# is:
	#	A:echo line 1
	#	B:echo line 2)" && prefix DASH_CODE <<DASH_CODE
	#	B:echo line 3
	#	line 4
	#	line 5
	#
	# The likely intended output is ...
	#
	#	A:echo line 3
	#	B:echo line 1
	#	line 2
	#	DASH_CODE:echo line 4)"
	#	DASH_CODE:echo line 5
	#
	# The difference is explained by differeng opinions on just
	# when processing of a here doc should start

	cat <<- \END_SCRIPT > script
		prefix() { sed -e "s/^/$1:/"; }
		DASH_CODE() { :; }

		prefix A <<XXX && echo "$(prefix B <<XXX
		echo line 1
		XXX
		echo line 2)" && prefix DASH_CODE <<DASH_CODE
		echo line 3
		XXX
		echo line 4)"
		echo line 5
		DASH_CODE
	END_SCRIPT

	# we will just verify that the shell can parse the
	# script somehow, and doesn't fall over completely...

	atf_check -s exit:0 -o ignore -e empty ${TEST+SH} script
}

atf_init_test_cases() {
	atf_add_test_case do_simple
	atf_add_test_case incomplete
	atf_add_test_case multiple	# multiple << operators on one cmd
	atf_add_test_case vicious	# evil test from the austin-l list...
}
