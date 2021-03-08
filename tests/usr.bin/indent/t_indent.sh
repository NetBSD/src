#! /bin/sh
# $NetBSD: t_indent.sh,v 1.4 2021/03/08 22:13:05 rillig Exp $
#
# Copyright 2016 Dell EMC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# $FreeBSD: head/usr.bin/indent/tests/functional_test.sh 314613 2017-03-03 20:15:22Z ngie $

SRCDIR=$(atf_get_srcdir)

check()
{
	local tc=${1}; shift

	local indent=$(atf_config_get usr.bin.indent.test_indent /usr/bin/indent)

	# All of the files need to be in the ATF sandbox in order for the tests
	# to pass.
	atf_check cp ${SRCDIR}/${tc}* .

	# Remove single-line comments that start with '$'.  This removes RCS
	# IDs, preventing them to be broken into several lines.  It also
	# allows for remarks that are only needed in either the input or the
	# output.  These removals affect the line numbers in the diffs.
	for fname in "$tc" "$tc.stdout" "$tc.stderr"; do
		if [ -f "$fname" ]; then
			atf_check -o "save:$fname.clean" \
			    sed -e '/^\/\*[[:space:]]$.*/d' "$fname"
		fi
	done

	local out_arg='empty'
	if [ -f "$tc.stdout.clean" ]; then
		out_arg="file:$tc.stdout.clean"
	fi

	local err_arg='empty'
	if [ -f "$tc.stderr.clean" ]; then
		err_arg="file:$tc.stderr.clean"
	fi

	# Make sure we don't implicitly use ~/.indent.pro from the test
	# host, for determinism purposes.
	local pro_arg='-npro'
	if [ -f "$tc.pro" ]; then
		pro_arg="-P$tc.pro"
	fi

	atf_check -s "exit:${tc##*.}" -o "$out_arg" -e "$err_arg" \
	    "$indent" "$pro_arg" < "$tc.clean"
}

add_testcase()
{
	local tc=${1}
	local tc_escaped word

	case "${tc%.*}" in
	*-*)
		local IFS="-+"
		for word in ${tc%.*}; do
			tc_escaped="${tc_escaped:+${tc_escaped}_}${word}"
		done
		;;
	*)
		tc_escaped=${tc%.*}
		;;
	esac

	atf_test_case ${tc_escaped}
	eval "${tc_escaped}_body() { check ${tc}; }"
	atf_add_test_case ${tc_escaped}
}

atf_init_test_cases()
{
	for path in $(find -Es "${SRCDIR}" -regex '.*\.[0-9]+$'); do
		add_testcase ${path##*/}
	done
}
