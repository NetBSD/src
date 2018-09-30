# $NetBSD: t_builtin.sh,v 1.1.2.3 2018/09/30 01:45:58 pgoyette Exp $
#
# Copyright (c) 2018 The NetBSD Foundation, Inc.
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

# The shell to use for tests of the builtin printf (or at least try)
: ${TEST_SH:=/bin/sh}

# This tests the builtin printf command in ${TEST_SH}

# For the actual code/tests see printf.sh
# (shared with tests for the external (filesystem) command printf

# These tests are designed to be able to be run by ATF, or standalone
#
# That is, either
#	atf_run t_builtin | atf-report		(or whatever is needed)
# or
#	sh t_builtin [sub_test]...	(default is to run all sub_tests)
#
# nb: for standalone runs, do not attempt ./t_builtin as the #! line
# added will force ATF which will complain about the test being run
# in the wrong way.  Instead use some Bourne shell compatible shell
# (any will do, caveat any bugs it might have, it need not be TEST_SH,
# but it can be) and run the script explicitly.

do_printf()
{
	$Running_under_ATF && atf_require_prog "${TEST_SH%% *}"

	unset LANG LC_ALL LC_NUMERIC LC_CTYPE LC_MESSAGES

	case "$*" in
	*\'*)
		$Running_under_ATF && atf_require_prog printf
		$Running_under_ATF && atf_require_pfog sed
		;;
	esac

	COMMAND=printf
	for ARG
	do
		case "${ARG}" in
		(';')	# Allow multiple commands
			COMMAND="${COMMAND} ; printf"
			;;
		(*\'*)
			# This is kind of odd, we need a working
			# printf in order to test printf ...
			# nb: do not use echo here, an arg might be "-n'x"
			COMMAND="${COMMAND} '$(
			    printf '%s\n' "${ARG}" |
				sed "s/'/'\\\\''/g"
			)'"
			;;
		(*)
			COMMAND="${COMMAND} '${ARG}'"
			;;
		esac
	done
	${TEST_SH} -c "${COMMAND}"
}

Not_builtin()
{
	if $Running_under_ATF
	then
		atf_skip "${TEST_SH%% *} does not have printf built in"
	else
		echo >&2 "No builtin printf in ${TEST_SH}"
		exit 1
	fi
}

# See if we have a builtin "printf" command to test

setup()
{
	# If the shell being used for its printf supports "type -t", use it
	if B=$( ${TEST_SH} -c 'type -t printf' 2>/dev/null )
	then
		case "$B" in
		( builtin )	return 0;;
		esac
	else
		# We get here if type -t is not supported, or it is,
		# but printf is completely unknown.  No harm trying again.

		case "$( unset LANG LC_ALL LC_NUMERIC LC_CTYPE LC_MESSAGES
		    ${TEST_SH} -c 'type printf' 2>&1 )" in
		( *[Bb]uiltin* | *[Bb]uilt[-\ ][Ii]n* ) return 0;;
		esac
	fi

	Tests=
	define Not_builtin 'Dummy test to skip when no printf builtin'
	return 0
}

setmsg()
{
	MSG="${TEST_SH%% *} builtin printf"
	CurrentTest="$1"
	RVAL=0
}

BUILTIN_TEST=true

# All the code to actually run the test comes from printf.sh ...

