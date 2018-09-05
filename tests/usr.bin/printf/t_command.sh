# $NetBSD: t_command.sh,v 1.1 2018/09/05 21:05:40 kre Exp $
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

# The printf command to test (if not a full path, $PATH will be examined)
# Do not use relative (./ or ../) paths when running under ATF.
: ${TEST_PRINTF:=/usr/bin/printf}

# This tests an external (filesystem) printf command

# For the actual code/tests see printf.sh
# (shared with tests for the shell builtin printf

# These tests are designed to be able to be run by ATF, or standalone
#
# That is, either
#	atf_run t_command | atf-report		(or whatever is needed)
# or
#	sh t_command [sub_test]...	( default is to run all sub_tests )
#
# nb: for standalone runs, do not attempt ./t_builtin as the #! line
# added will force ATF which will complain about the incorrect method
# of running the test.  Instead use some Bourne shell compatible shell
# (any will do, caveat any bugs it might have), and run the script explicitly.

do_printf()
{
	$Running_under_ATF && atf_require_prog "${PRINTF%% *}"

	unset LANG LC_ALL LC_NUMERIC LC_CTYPE LC_MESSAGES

	${PRINTF} "$@"
}

No_command()
{
	setmsg No_command

	case "${TEST_PRINTF%% *}" in
	( '' )
		msg='Configuration error: check $TEST_PRINTF'
		;;
	( /* | ./* | ../* )
		msg="Cannot find/execute ${TEST_PRINTF%% *}"
		;;
	( * )
		msg="No '${TEST_PRINTF%% *}' found in "'$PATH'
		;;
	esac
	atf_skip "${msg}"

	return $RVAL
}

# See if we have a "printf" command in $PATH to test - pick the first

setup()
{
	saveIFS="${IFS-UnSet}"
	saveOPTS="$(set +o)"

	unset PRINTF

	case "${TEST_PRINTF%% *}" in
	( /* )		PRINTF="${TEST_PRINTF}" ;;
	( ./* | ../* )	PRINTF="${PWD}/${TEST_PRINTF}" ;;
	(*)
		set -f
		IFS=:
		for P in $PATH
		do
			case "$P" in
			('' | . )	D="${PWD}";;
			( /* )		D="${P}"  ;;
			( * )		D="${PWD}/${P}";;
			esac

			test -x "${D}/${TEST_PRINTF%% *}" || continue

			PRINTF="${D}/${TEST_PRINTF}"
			break
		done
		unset IFS
		eval "${saveOPTS}"

		case "${saveIFS}" in
		(UnSet)		unset IFS;;
		(*)		IFS="${saveIFS}";;
		esac
		;;
	esac

	test -x "${PRINTF%% *}" || PRINTF=

	case "${PRINTF}" in

	('')	Tests=
		define No_command 'Dummy test to skip no printf command'
		return 1
		;;

	( * )
		# nothing here, it all happens below.
		;;

	esac

	return 0
}

setmsg()
{
	MSG="${PRINTF}"
	CurrentTest="$1"
	RVAL=0
}

BUILTIN_TEST=false

# All the code to actually run the test comes from printf.sh ...

