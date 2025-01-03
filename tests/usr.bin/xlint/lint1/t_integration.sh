# $NetBSD: t_integration.sh,v 1.85 2025/01/03 02:14:52 rillig Exp $
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

: "${lint1:=/usr/libexec/lint1}"
: "${archsubdir:=archsubdir_must_be_set}"

srcdir="$(atf_get_srcdir)"

configure_test_case()
{
	local awk

	# shellcheck disable=SC2016
	awk='
		BEGIN {
			# see ./gen-platforms.sh
			platform["aarch64"]	= "uchar lp64  long ldbl128"
			platform["alpha"]	= "schar lp64  long ldbl64"
			platform["arm"]		= "uchar ilp32 long ldbl64"
			platform["coldfire"]	= "schar ilp32 int  ldbl64"
			platform["hppa"]	= "schar ilp32 long ldbl64"
			platform["i386"]	= "schar ilp32 int  ldbl96"
			platform["ia64"]	= "schar lp64  long ldbl128"
			platform["m68000"]	= "schar ilp32 int  ldbl64"
			platform["m68k"]	= "schar ilp32 int  ldbl96"
			platform["mips"]	= "schar ilp32 ???? ldbl64"
			platform["mips64"]	= "schar ilp32 long ldbl128"
			platform["mipsn64"]	= "schar lp64  long ldbl128"
			platform["or1k"]	= "schar ilp32 int  ldbl64"
			platform["powerpc"]	= "uchar ilp32 int  ldbl64"
			platform["powerpc64"]	= "uchar lp64  long ldbl64"
			platform["riscv32"]	= "schar ilp32 int  ldbl64"
			platform["riscv64"]	= "schar lp64  long ldbl64"
			platform["sh3"]		= "schar ilp32 int  ldbl64"
			platform["sparc"]	= "schar ilp32 long ldbl64"
			platform["sparc64"]	= "schar lp64  long ldbl128"
			platform["vax"]		= "schar ilp32 long ldbl64"
			platform["x86_64"]	= "schar lp64  long ldbl128"
		}

		function platform_has(prop) {
			if (!match(prop, /^(schar|uchar|ilp32|lp64|int|long|ldbl64|ldbl96|ldbl128)$/)) {
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

	case "_${1%.c}_" in
	*_utf8_*)
		LC_ALL=en_US.UTF-8;;
	*)
		LC_ALL=C;;
	esac
	export LC_ALL
}

tests_done=''
check_lint1()
{
	local src="$1"
	local base="${src##*/}"
	local exp="${base%.c}.exp"
	local exp_ln="${src%.c}.exp-ln"
	local wrk_ln="${base%.c}.ln"
	local flags=""
	local skip=""

	if [ ! -f "$exp_ln" ]; then
		exp_ln='/dev/null'
		wrk_ln='/dev/null'
	fi

	configure_test_case "$src"	# sets 'skip' and 'flags'

	if [ "$skip" = "yes" ]; then
		return
	fi
	tests_done="$tests_done $src"

	# shellcheck disable=SC2086
	atf_check -s 'exit' -o "save:$exp" \
	    "$lint1" $flags "$src" "$wrk_ln"

	if [ "$exp_ln" != '/dev/null' ]; then
		# Remove comments and whitespace from the .exp-ln file.
		sed \
		    -e '/^#/d' \
		    -e '/^$/d' \
		    -e 's,^#.*,,' \
		    -e 's,\([^%]\)[[:space:]],\1,g' \
		    < "$exp_ln" > "./${exp_ln##*/}"

		atf_check -o "file:${exp_ln##*/}" cat "$wrk_ln"
	fi
}

atf_test_case lint1
lint1_head() {
	atf_set 'require.progs' "$lint1"
}
lint1_body() {
	local src

	for src in "$srcdir"/*.c; do
		check_lint1 "$src"
	done

	# shellcheck disable=SC2086
	atf_check lua "$srcdir/check-expect.lua" $tests_done
}

atf_init_test_cases()
{
	atf_add_test_case lint1
}
