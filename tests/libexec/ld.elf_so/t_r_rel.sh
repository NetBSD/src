#	$NetBSD: t_r_rel.sh,v 1.3 2025/12/21 19:08:09 riastradh Exp $
#
# Copyright (c) 2025 The NetBSD Foundation, Inc.
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

cleanup_core()
{
	local prog

	prog=$1
	test -f "${prog}.core" || return 0
	readelf -rs "$(atf_get_srcdir)/${prog}"
	gdb -batch -ex bt -ex 'info registers' -ex disas \
	    "$(atf_get_srcdir)/${prog}" "${prog}.core"
}

atf_test_case readelf_relative_nopack
readelf_relative_nopack_head()
{
	atf_set "descr" "readelf R_*_RELATIVE with -z nopack-relative-relocs"
	atf_set "require.progs" "readelf"
}
readelf_relative_nopack_body()
{
	atf_check -o match:'R_.*_REL' \
	    readelf -r "$(atf_get_srcdir)"/h_r_rel_nopack
}

atf_test_case readelf_relative_pack
readelf_relative_pack_head()
{
	atf_set "descr" "readelf R_*_RELATIVE with -z pack-relative-relocs"
	atf_set "require.progs" "readelf"
}
readelf_relative_pack_body()
{
	case `uname -p` in
	i386|powerpc64*|x86_64)
		;;
	*)	# Actually missing GNU binutils ld(1) support.
		atf_expect_fail "PR bin/59360: ld.elf_so(8):" \
		    " missing RELR support"
		;;
	esac
	atf_check -o not-match:'R_.*_REL' \
	    readelf -r "$(atf_get_srcdir)"/h_r_rel_pack
}

atf_test_case run_relative_nopack cleanup
run_relative_nopack_head()
{
	atf_set "descr" "run R_*_RELATIVE with -z nopack-relative-relocs"
}
run_relative_nopack_body()
{
	atf_check "$(atf_get_srcdir)"/h_r_rel_nopack
}
run_relative_nopack_cleanup()
{
	cleanup_core h_r_rel_nopack
}

atf_test_case run_relative_pack cleanup
run_relative_pack_head()
{
	atf_set "descr" "run R_*_RELATIVE with -z pack-relative-relocs"
}
run_relative_pack_body()
{
	case `uname -p` in
	i386|powerpc64*|x86_64)
		;;
	*)	# Missing GNU binutils ld(1) support to generate RELR
		# sections, so the program should run just fine because
		# it just uses traditional REL/RELA instead.
		;;
	esac
	atf_check "$(atf_get_srcdir)"/h_r_rel_pack
}
run_relative_pack_cleanup()
{
	cleanup_core h_r_rel_pack
}

atf_init_test_cases()
{
	atf_add_test_case readelf_relative_nopack
	atf_add_test_case readelf_relative_pack
	atf_add_test_case run_relative_nopack
	atf_add_test_case run_relative_pack
}
