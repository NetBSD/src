#! /bin/sh
# $NetBSD: accept.sh,v 1.9 2022/04/16 09:22:25 rillig Exp $
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

# usage: accept.sh <pattern>...
#
#	Accept the actual output from running the lint tests and save them
#	back into the .exp files.

set -eu

: "${archsubdir:=$(make -v ARCHSUBDIR)}"
. './t_integration.sh'		# for configure_test_case

for pattern in "$@"; do
	# shellcheck disable=SC2231
	for test in *$pattern*.c; do
		base=${test%.*}
		cfile="$base.c"
		expfile="$base.exp"
		tmpfile="$base.exp.tmp"
		ln_file="$base.exp-ln"

		configure_test_case "$cfile"
		# shellcheck disable=SC2154
		if [ "$skip" = yes ]; then
			continue
		fi

		if [ ! -f "$ln_file" ]; then
			ln_file='/dev/null'
		fi

		# shellcheck disable=SC2154
		# shellcheck disable=SC2086
		if "$lint1" $flags "$base.c" "$ln_file" > "$tmpfile"; then
			if [ -s "$tmpfile" ]; then
				echo "$base produces output but exits successfully"
				sed 's,^,| ,' "$tmpfile"
			fi
			rm -f "$expfile" "$tmpfile"
		elif [ $? -ge 128 ]; then
			echo "$base crashed"
			continue
		else
			if [ -f "$tmpfile" ] && cmp -s "$tmpfile" "$expfile"; then
				rm "$tmpfile"
			else
				echo "replacing $base"
				mv "$tmpfile" "$expfile"
			fi
		fi

		case "$base" in (msg_*)
			if grep 'This message is not used\.' "$cfile" >/dev/null; then
				: 'Skip further checks.'
			elif [ ! -f "$expfile" ]; then
				echo "$base should produce warnings"
			elif grep '^TODO: "Add example code' "$base.c" >/dev/null; then
				: 'ok, this test is not yet written'
			else
				msgid=${base}
				msgid=${msgid#msg_00}
				msgid=${msgid#msg_0}
				msgid=${msgid#msg_}
				msgid=${msgid%%_*}
				if ! grep "\\[$msgid\\]" "$expfile" >/dev/null; then
					echo "$base should trigger the message '$msgid'"
				fi
			fi
		esac
	done
done

# shellcheck disable=SC2035
lua '../check-expect.lua' *.c
