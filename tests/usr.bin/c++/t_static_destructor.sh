#	$NetBSD: t_static_destructor.sh,v 1.10 2025/11/05 18:27:24 christos Exp $
#
# Copyright (c) 2017 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Kamil Rytarowski.
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

mktest() {
	cat > $@.cpp << EOF
#include <iostream>
struct A {
	int i;
	A(int i):i(i){std::cout << "CTOR A" << std::endl;}
	~A() {std::cout << "DTOR A:" << i << std::endl;}
};
struct B {
	A *m_a;
	B(){static A s_a(10);m_a=&s_a;std::cout << "CTOR B" << std::endl;}
	~B(){std::cout << "DTOR B:" << (*m_a).i << std::endl;(*m_a).i = 20;}
};
int $@(void) {struct B b;return 0;}
EOF
}

check() {
	atf_check -s exit:0 -o inline:"CTOR A\nCTOR B\nDTOR B:10\nDTOR A:20\n" $@
}

ccmain() {
	atf_check -s exit:0 -o ignore -e ignore c++ $@ -o main main.cpp
	check ./main
}


mkmain() {
	cat > main.cpp << EOF
#include <cstdlib>
int $@(void);
int main(void) {$@();exit(0);}
EOF
}

cclib() {
	atf_check -s exit:0 -o ignore -e ignore \
	    c++ "$@" -fPIC -shared -o libpic.so pic.cpp
}

check32() {
	# check whether this arch is 64bit
	if ! c++ -dM -E - < /dev/null | fgrep -q _LP64; then
		atf_skip "this is not a 64 bit architecture"
		return 1
	fi
	if ! c++ -m32 -dM -E - < /dev/null 2>/dev/null > ./def32; then
		atf_skip "c++ -m32 not supported on this architecture"
		return 1
	else
		if fgrep -q _LP64 ./def32; then
			atf_fail "c++ -m32 does not generate netbsd32 binaries"
			return 1
		fi
	fi
	return 0
}

check59301() {
	case `uname -m` in
	riscv)	atf_expect_fail "PR port-riscv/59301:" \
		    " riscv: missing MKPROFILE=yes support"
		return 1
		;;
	esac
	return 0
}

atf_test_case static_destructor
static_destructor_head() {
	atf_set "descr" "ccmain and run \"hello world\""
	atf_set "require.progs" "c++"
}

atf_test_case static_destructor_profile
static_destructor_profile_head() {
	atf_set "descr" "ccmain and run \"hello world\" with profiling option"
	atf_set "require.progs" "c++"
}

atf_test_case static_destructor_static
static_destructor_static_head() {
	atf_set "descr" "ccmain and run \"hello world\" with static option"
	atf_set "require.progs" "c++"
}

atf_test_case static_destructor_pic
static_destructor_pic_head() {
	atf_set "descr" "ccmain and run PIC \"hello world\""
	atf_set "require.progs" "c++"
}

atf_test_case static_destructor_pic_32
static_destructor_pic_32_head() {
	atf_set "descr" "ccmain and run 32-bit PIC \"hello world\""
	atf_set "require.progs" "c++"
}

atf_test_case static_destructor_pic_profile
static_destructor_pic_profile_head() {
	atf_set "descr" "ccmain and run PIC \"hello world\" with profiling option"
	atf_set "require.progs" "c++"
}

atf_test_case static_destructor_pic_profile_32
static_destructor_pic_profile_32_head() {
	atf_set "descr" "ccmain and run 32-bit PIC \"hello world\" with profiling option"
	atf_set "require.progs" "c++"
}

atf_test_case static_destructor_profile_32
static_destructor_profile_32_head() {
	atf_set "descr" "ccmain and run 32-bit \"hello world\" with profiling option"
	atf_set "require.progs" "c++"
}

atf_test_case static_destructor_pie
static_destructor_pie_head() {
	atf_set "descr" "ccmain and run position independent (PIE) \"hello world\""
	atf_set "require.progs" "c++"
}

atf_test_case static_destructor32
static_destructor32_head() {
	atf_set "descr" "ccmain and run \"hello world\" for/in netbsd32 emulation"
	atf_set "require.progs" "c++ file diff cat"
}

static_destructor_body() {
	mktest main
	ccmain
}

static_destructor_profile_body() {
	check59301 || return

	mktest main
	ccmain -static -pg
}

static_destructor_profile_32_body() {
	check32 || return
	check59301 || return

	mktest main
	ccmain -static -pg -m32
}


static_destructor_static_body() {
	mktest main
	ccmain -static
}

static_destructor_pic_body() {
	mktest pic
	mkmain pic
	cclib
	ccmain -L${PWD} -Wl,-R${PWD} -lpic
}

static_destructor_pic_32_body() {
	check32 || return
	mktest pic
	mkmain pic
	cclib -m32
	ccmain -m32 -L${PWD} -Wl,-R${PWD} -lpic
}

static_destructor_pic_profile_body() {
	check59301 || return

	mktest pic
	mkmain pic
	cclib -pg 
	ccmain -pg -L${PWD} -Wl,-R${PWD} -lpic
}

static_destructor_pic_profile_32_body() {
	check32 || return
	check59301 || return

	mktest pic
	mkmain pic
	cclib -m32 -pg 
	ccmain -m32 -pg -L${PWD} -Wl,-R${PWD} -lpic
}

static_destructor_pie_body() {
	# check whether this arch supports -pie
	if ! c++ -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then
		atf_skip "c++ -pie not supported on this architecture"
	fi
	mktest main
	ccmain -fpie -pie 
}

static_destructor32_body() {
	check32 || return

	mktest main
	atf_check -s exit:0 -o ignore -e ignore c++ -o hello32 -m32 main.cpp
	atf_check -s exit:0 -o ignore -e ignore c++ -o hello64 main.cpp
	file -b ./hello32 > ./ftype32
	file -b ./hello64 > ./ftype64
	if diff ./ftype32 ./ftype64 >/dev/null; then
		atf_fail "generated binaries do not differ"
	fi
	echo "32bit binaries on this platform are:"
	cat ./ftype32
	echo "While native (64bit) binaries are:"
	cat ./ftype64
	check ./hello32
	atf_check -s exit:0 -o ignore -e ignore c++ -o hello -m32 \
	    -static main.cpp
	check ./hello
}

atf_init_test_cases()
{

	atf_add_test_case static_destructor
	atf_add_test_case static_destructor_profile
	atf_add_test_case static_destructor_pic
	atf_add_test_case static_destructor_pie
	atf_add_test_case static_destructor32
	atf_add_test_case static_destructor_static
	atf_add_test_case static_destructor_pic_32
	atf_add_test_case static_destructor_pic_profile
	atf_add_test_case static_destructor_pic_profile_32
	atf_add_test_case static_destructor_profile_32
}
