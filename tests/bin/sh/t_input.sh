# $NetBSD: t_input.sh,v 1.1 2021/02/16 09:46:24 kre Exp $
#
# Copyright (c) 2021 The NetBSD Foundation, Inc.
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

# This set of tests checks the low level shell (script) input
# systrem (reading script files, nested files, fines read while
# reading strings, ..) and correctly dropping nul bytes from file data

# other shell input (the read builtin for example) is not covered here.

atf_test_case nul_elimination

nul_elimination_head() {
	atf_set "descr" "verifies that \0 chars in input are properly ignored"
}

nul_elimination_body() {
	atf_require_prog printf
	atf_require_prog stat

	# these really should always be present, but...
	atf_require_prog dd
	atf_require_prog cat
	atf_require_prog rm

	# please do not make even trivial changes (like correcting spelling)
	# to the following script without taking care to fix the following
	# tests, even minor changes can defeat the purpose of the test
	cat > helper.sh <<- EOF
		# a line of commentary that does nothing
		# another line of comments (these just make the script bigger)
		printf A
		eval "printf B"
		if printf C
		then
			printf D
		fi
		for x in E F G H
		do
			printf "\$x"
		done
		printf \\
			I
		printf '\n'
		exit 0
	EOF

	# this first test simply verifies that the script works as
	# expected, should it fail, it is not a problem with \0 chars
	atf_check -s exit:0 -o 'inline:ABCDEFGHI\n' -e empty \
		${TEST_SH} helper.sh

	size=$(stat -f %z helper.sh)

	# Now insert \0 chars at specifically chosen spots in script
	for loc in 0 10 40 41 42 104 105 106 110 111 112 113 119 127 128 \
		144 150 162 165 168 187 202 203 204 205 214 215 216 224 225
	do
		# at each location try varying the number of \0's inserted
		for N in 1 2 4 7
		do
			OF="helper-${N}@${loc}.sh"

			test "${loc}" -gt 0 && 
				dd if=helper.sh bs=1 count="${loc}" \
				   of="${OF}" >/dev/null 2>&1
			dd if=helper.sh bs=1 skip="${loc}" \
				oseek="$(( loc + N ))" \
				of="${OF}" >/dev/null 2>&1

			test "$(stat -f %z "${OF}")" -eq  "$(( size + N ))" ||
					atf_fail "${N}@${loc}: build failure"

			atf_check -s exit:0 -o 'inline:ABCDEFGHI\n' -e empty \
				${TEST_SH} "${OF}"

			rm -f "${OF}"
		done
			
	done

	# Next insert \0 chars at multiple chosen locations
	# nb: offsets in each arg must be non-decreasing
	for locs in '0 225' '0 224' '0 127 223' '0 10 15' '0 48 55' \
		'0 0 0 225 225 225' '0 0 127 128 224 224 225' \
		'104 106' '105 110' '113 119' '113 119 122' '113 127' \
		'129 130 131 132 133 136 139 140'  '184 185 186 187' \
		'202 203 203 204 204 205 205 206 207'
	do
		set -- $locs
		gaps=$#

		IFS=-
		OF="helper-${*}.sh"
		unset IFS

		loc=$1; shift
		test "${loc}" -gt 0 && 
			dd if=helper.sh bs=1 count="${loc}" \
			   of="${OF}" >/dev/null 2>&1 
		G=1
		for N
		do
			count=$(( N - loc ))
			if [ "${count}" -eq 0 ]
			then
				printf '\0' >> "${OF}"
			else
				dd if=helper.sh bs=1 skip="${loc}" \
				    oseek="$(( loc + G ))" count="${count}" \
				    of="${OF}" >/dev/null 2>&1
			fi
			loc=$N
			G=$(( G + 1 ))
		done

		if [ "${loc}" -lt "${size}" ]
		then
			dd if=helper.sh bs=1 skip="${loc}" \
			    oseek="$(( loc + G ))" of="${OF}" >/dev/null 2>&1
		else
			printf '\0' >> "${OF}"
		fi

		test "$(stat -f %z "${OF}")" -eq  "$(( size + gaps ))" ||
				atf_fail "${locs}: build failure"

		atf_check -s exit:0 -o 'inline:ABCDEFGHI\n' -e empty \
			${TEST_SH} "${OF}"

		rm -f "${OF}"
	done

	# Now just insert \0's in random locations in the script,
	# hoping that if the somewhat carefully selected insertions
	# above fail to trigger a bug, then if any (bugs) remain,
	# eventually Murphy will prevail, and one of these tests will catch it.

	test "${RANDOM}" = "${RANDOM}" &&
		atf_skip 'ATF shell does not support $RANDOM'

	# To be added later...

	return 0
}

atf_init_test_cases() {
	atf_add_test_case nul_elimination
	# atf_add_test_case file_recursion
	# atf_add_test_case file_string_recursion
	# atf_add_test_case file_recursion_nul
	# atf_add_test_case file_string_recursion_nul
}
