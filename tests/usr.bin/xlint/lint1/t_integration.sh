# $NetBSD: t_integration.sh,v 1.64 2021/06/29 08:46:10 rillig Exp $
#
# Copyright (c) 2008, 2010 The NetBSD Foundation, Inc.
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

lint1=/usr/libexec/lint1

machine_arch="$(sysctl -n hw.machine_arch)"


configure_test_case()
{
	local awk

	# shellcheck disable=SC2016
	awk='
		function is_ilp32() {
			return match(machine_arch, /^(arm|coldfire|hppa|i386|m68000|m68k|mips|mips64|or1k|powerpc|riscv32|sh3|sparc|vax)$/)
		}

		function is_lp64() {
			return match(machine_arch, /^(aarch64|alpha|ia64|mipsn64|powerpc64|riscv64|sparc64|x86_64)$/)
		}

		BEGIN {
			machine_arch = "'"$machine_arch"'"
			flags = "-g -S -w"
			seen_only_on_arch = 0
			match_only_on_arch = 0
			skip = 0
		}
		$1 == "/*" && $2 ~ /^lint1-/ && $NF == "*/" {
			if ($2 == "lint1-flags:" || $2 == "lint1-extra-flags:") {
				if ($2 == "lint1-flags:")
					flags = ""
				for (i = 3; i < NF; i++)
					flags = flags " " $i
			}
			if ($2 == "lint1-only-on-arch") {
				seen_only_on_arch = 1
				if ($3 == machine_arch)
					match_only_on_arch = 1
			}
			if ($2 == "lint1-not-on-arch" && $3 == machine_arch)
				skip = 1
			if ($2 == "lint1-only-on-ilp32" && !is_ilp32())
				skip = 1
			if ($2 == "lint1-only-on-lp64" && !is_lp64())
				skip = 1
		}

		END {
			if (seen_only_on_arch && !match_only_on_arch)
				skip = 1

			printf("flags='\''%s'\''\n", flags)
			printf("skip=%s\n", skip ? "yes" : "no")
		}
	'

	eval "$(awk "$awk" "$1")"
}

# shellcheck disable=SC2155
check_lint1()
{
	local src="$(atf_get_srcdir)/$1"
	local exp="${src%.c}.exp"
	local exp_ln="${src%.c}.exp-ln"
	local wrk_ln="${1%.c}.ln"
	local flags=""
	local skip=""

	if [ ! -f "$exp_ln" ]; then
		exp_ln='/dev/null'
		wrk_ln='/dev/null'
	fi

	configure_test_case "$src"

	if [ "$skip" = "yes" ]; then
		atf_check -o 'ignore' echo 'skipped'
		return
	fi

	if [ -f "$exp" ]; then
		# shellcheck disable=SC2086
		atf_check -s not-exit:0 -o "file:$exp" -e empty \
		    "$lint1" $flags "$src" "$wrk_ln"
	else
		# shellcheck disable=SC2086
		atf_check -s exit:0 \
		    "$lint1" $flags "$src" "$wrk_ln"
	fi

	if [ "$exp_ln" != '/dev/null' ]; then
		atf_check -o "file:$exp_ln" cat "$wrk_ln"
	fi
}

atf_init_test_cases()
{
	local src name

	for src in "$(atf_get_srcdir)"/*.c; do
		src=${src##*/}
		name=${src%.c}

		atf_test_case "$name"
		eval "${name}_head() {
			atf_set 'require.progs' '$lint1'
		}"
		eval "${name}_body() {
			check_lint1 '$name.c'
		}"
		atf_add_test_case "$name"
	done
}
