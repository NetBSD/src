# $NetBSD: t_integration.sh,v 1.75 2022/02/26 20:36:11 rillig Exp $
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
: "${archsubdir:=archsubdir_must_be_set}"


configure_test_case()
{
	local awk

	# shellcheck disable=SC2016
	awk='
		BEGIN {
			# see usr.bin/xlint/arch/*/targparam.h
			platform["aarch64"]	= "uchar lp64  long ldbl-128"
			platform["alpha"]	= "schar lp64  long ldbl-64"
			platform["arm"]		= "uchar ilp32 long ldbl-64"
			platform["coldfire"]	= "schar ilp32 int  ldbl-64"
			platform["hppa"]	= "schar ilp32 long ldbl-64"
			platform["i386"]	= "schar ilp32 int  ldbl-96"
			platform["ia64"]	= "schar lp64  long ldbl-128"
			platform["m68000"]	= "schar ilp32 int  ldbl-64"
			platform["m68k"]	= "schar ilp32 int  ldbl-96"
			platform["mips"]	= "schar ilp32 ???? ldbl-64"
			platform["mips64"]	= "schar ilp32 long ldbl-128"
			platform["mipsn64"]	= "schar lp64  long ldbl-128"
			platform["or1k"]	= "schar ilp32 int  ldbl-64"
			platform["powerpc"]	= "uchar ilp32 int  ldbl-64"
			platform["powerpc64"]	= "uchar lp64  long ldbl-64"
			platform["riscv32"]	= "schar ilp32 int  ldbl-64"
			platform["riscv64"]	= "schar lp64  long ldbl-64"
			platform["sh3"]		= "schar ilp32 int  ldbl-64"
			platform["sparc"]	= "schar ilp32 long ldbl-64"
			platform["sparc64"]	= "schar lp64  long ldbl-128"
			platform["vax"]		= "schar ilp32 long ldbl-64"
			platform["x86_64"]	= "schar lp64  long ldbl-128"
		}

		function platform_has(prop) {
			if (!match(prop, /^(schar|uchar|ilp32|lp64|int|long|ldbl-64|ldbl-96|ldbl-128)$/)) {
				printf("bad property '\''%s'\''\n", prop) > "/dev/stderr"
				exit(1)
			}
			if (platform[archsubdir] == "") {
				printf("bad archsubdir '\''%s'\''\n", archsubdir) > "/dev/stderr"
				exit(1)
			}
			return match(" " platform[archsubdir] " ", " " prop " ")
		}

		BEGIN {
			archsubdir = "'"$archsubdir"'"
			flags = "-g -S -w"
			skip = "no"
		}
		$1 == "/*" && $2 ~ /^lint1-/ && $NF == "*/" {
			if ($2 == "lint1-flags:" || $2 == "lint1-extra-flags:") {
				if ($2 == "lint1-flags:")
					flags = ""
				for (i = 3; i < NF; i++)
					flags = flags " " $i
			} else if ($2 == "lint1-only-if:") {
				for (i = 3; i < NF; i++)
					if (!platform_has($i))
						skip = "yes"
			} else if ($2 == "lint1-skip-if:") {
				for (i = 3; i < NF; i++)
					if (platform_has($i))
						skip = "yes"
			} else {
				printf("bad lint1 comment '\''%s'\''\n", $2) > "/dev/stderr"
				exit(1)
			}
		}

		END {
			printf("flags='\''%s'\''\n", flags)
			printf("skip=%s\n", skip)
		}
	'

	local config
	config="$(awk "$awk" "$1")" || exit 1
	eval "$config"
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

	configure_test_case "$src"	# sets 'skip' and 'flags'

	if [ "$skip" = "yes" ]; then
		atf_skip "unsuitable platform"
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
