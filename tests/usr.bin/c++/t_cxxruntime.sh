#	$NetBSD: t_cxxruntime.sh,v 1.6 2022/06/12 08:55:36 skrll Exp $
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

atf_test_case cxxruntime
cxxruntime_head() {
	atf_set "descr" "compile and run \"hello world\""
	atf_set "require.progs" "c++"
}

atf_test_case cxxruntime_profile
cxxruntime_profile_head() {
	atf_set "descr" "compile and run \"hello world\" with profiling option"
	atf_set "require.progs" "c++"
}

atf_test_case cxxruntime_profile_32
cxxruntime_profile_32_head() {
	atf_set "descr" "compile and run 32-bit \"hello world\" with profiling option"
	atf_set "require.progs" "c++"
}

atf_test_case cxxruntime_static
cxxruntime_static_head() {
	atf_set "descr" "compile and run \"hello world\" with static flags"
	atf_set "require.progs" "c++"
}

atf_test_case cxxruntime_pic
cxxruntime_pic_head() {
	atf_set "descr" "compile and run PIC \"hello world\""
	atf_set "require.progs" "c++"
}

atf_test_case cxxruntime_pic_32
cxxruntime_pic_32_head() {
	atf_set "descr" "compile and run 32-bit PIC \"hello world\""
	atf_set "require.progs" "c++"
}

atf_test_case cxxruntime_pic_profile
cxxruntime_pic_profile_head() {
	atf_set "descr" "compile and run PIC \"hello world\" with profiling option"
	atf_set "require.progs" "c++"
}

atf_test_case cxxruntime_pic_profile_32
cxxruntime_pic_profile_32_head() {
	atf_set "descr" "compile and run 32-bit PIC \"hello world\" with profiling option"
	atf_set "require.progs" "c++"
}

atf_test_case cxxruntime_pie
cxxruntime_pie_head() {
	atf_set "descr" "compile and run position independent (PIE) \"hello world\""
	atf_set "require.progs" "c++"
}

atf_test_case cxxruntime32
cxxruntime32_head() {
	atf_set "descr" "compile and run \"hello world\" for/in netbsd32 emulation"
	atf_set "require.progs" "c++ file diff cat"
}

cxxruntime_body() {
	cat > test.cpp << EOF
#include <cstdlib>
#include <iostream>
int main(void) {std::cout << "hello world" << std::endl;exit(0);}
EOF
	atf_check -s exit:0 -o ignore -e ignore c++ -o hello test.cpp
	atf_check -s exit:0 -o inline:"hello world\n" ./hello
}

cxxruntime_profile_body() {
	cat > test.cpp << EOF
#include <cstdlib>
#include <iostream>
int main(void) {std::cout << "hello world" << std::endl;exit(0);}
EOF
	atf_check -s exit:0 -static -o ignore -e ignore c++ -pg -o hello test.cpp
	atf_check -s exit:0 -o inline:"hello world\n" ./hello
}

cxxruntime_profile_32_body() {
	# check whether this arch is 64bit
	if ! c++ -dM -E - < /dev/null | fgrep -q _LP64; then
		atf_skip "this is not a 64 bit architecture"
	fi
	if ! c++ -m32 -dM -E - < /dev/null 2>/dev/null > ./def32; then
		atf_skip "c++ -m32 not supported on this architecture"
	else
		if fgrep -q _LP64 ./def32; then
			atf_fail "c++ -m32 does not generate netbsd32 binaries"
		fi
	fi

	cat > test.cpp << EOF
#include <cstdlib>
#include <iostream>
int main(void) {std::cout << "hello world" << std::endl;exit(0);}
EOF
	atf_check -s exit:0 -static -o ignore -e ignore c++ -m32 -pg -o hello test.cpp
	atf_check -s exit:0 -o inline:"hello world\n" ./hello
}

cxxruntime_static_body() {
	cat > test.cpp << EOF
#include <cstdlib>
#include <iostream>
int main(void) {std::cout << "hello world" << std::endl;exit(0);}
EOF
	atf_check -s exit:0 -o ignore -e ignore c++ -static -o hello test.cpp
	atf_check -s exit:0 -o inline:"hello world\n" ./hello
}

cxxruntime_pic_body() {
	cat > test.cpp << EOF
#include <cstdlib>
int callpic(void);
int main(void) {callpic();exit(0);}
EOF
	cat > pic.cpp << EOF
#include <iostream>
int callpic(void) {std::cout << "hello world" << std::endl;return 0;}
EOF

	atf_check -s exit:0 -o ignore -e ignore \
	    c++ -fPIC -shared -o libtest.so pic.cpp
	atf_check -s exit:0 -o ignore -e ignore \
	    c++ -o hello test.cpp -L. -ltest

	export LD_LIBRARY_PATH=.
	atf_check -s exit:0 -o inline:"hello world\n" ./hello
}

cxxruntime_pic_32_body() {
	# check whether this arch is 64bit
	if ! c++ -dM -E - < /dev/null | fgrep -q _LP64; then
		atf_skip "this is not a 64 bit architecture"
	fi
	if ! c++ -m32 -dM -E - < /dev/null 2>/dev/null > ./def32; then
		atf_skip "c++ -m32 not supported on this architecture"
	else
		if fgrep -q _LP64 ./def32; then
			atf_fail "c++ -m32 does not generate netbsd32 binaries"
		fi
	fi

	cat > test.cpp << EOF
#include <cstdlib>
int callpic(void);
int main(void) {callpic();exit(0);}
EOF
	cat > pic.cpp << EOF
#include <iostream>
int callpic(void) {std::cout << "hello world" << std::endl;return 0;}
EOF

	atf_check -s exit:0 -o ignore -e ignore \
	    c++ -m32 -fPIC -shared -o libtest.so pic.cpp
	atf_check -s exit:0 -o ignore -e ignore \
	    c++ -m32 -o hello test.cpp -L. -ltest

	export LD_LIBRARY_PATH=.
	atf_check -s exit:0 -o inline:"hello world\n" ./hello
}

cxxruntime_pic_profile_body() {
	cat > test.cpp << EOF
#include <cstdlib>
int callpic(void);
int main(void) {callpic();exit(0);}
EOF
	cat > pic.cpp << EOF
#include <iostream>
int callpic(void) {std::cout << "hello world" << std::endl;return 0;}
EOF

	atf_check -s exit:0 -o ignore -e ignore \
	    c++ -pg -fPIC -shared -o libtest.so pic.cpp
	atf_check -s exit:0 -o ignore -e ignore \
	    c++ -pg -o hello test.cpp -L. -ltest

	export LD_LIBRARY_PATH=.
	atf_check -s exit:0 -o inline:"hello world\n" ./hello
}

cxxruntime_pic_profile_32_body() {
	# check whether this arch is 64bit
	if ! c++ -dM -E - < /dev/null | fgrep -q _LP64; then
		atf_skip "this is not a 64 bit architecture"
	fi
	if ! c++ -m32 -dM -E - < /dev/null 2>/dev/null > ./def32; then
		atf_skip "c++ -m32 not supported on this architecture"
	else
		if fgrep -q _LP64 ./def32; then
			atf_fail "c++ -m32 does not generate netbsd32 binaries"
		fi
	fi

	cat > test.cpp << EOF
#include <cstdlib>
int callpic(void);
int main(void) {callpic();exit(0);}
EOF
	cat > pic.cpp << EOF
#include <iostream>
int callpic(void) {std::cout << "hello world" << std::endl;return 0;}
EOF

	atf_check -s exit:0 -o ignore -e ignore \
	    c++ -m32 -pg -fPIC -shared -o libtest.so pic.cpp
	atf_check -s exit:0 -o ignore -e ignore \
	    c++ -m32 -pg -o hello test.cpp -L. -ltest

	export LD_LIBRARY_PATH=.
	atf_check -s exit:0 -o inline:"hello world\n" ./hello
}

cxxruntime_pie_body() {
	# check whether this arch supports -pie
	if ! c++ -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then
		atf_skip "c++ -pie not supported on this architecture"
	fi
	cat > test.cpp << EOF
#include <cstdlib>
#include <iostream>
int main(void) {std::cout << "hello world" << std::endl;exit(0);}
EOF
	atf_check -s exit:0 -o ignore -e ignore c++ -fpie -pie -o hello test.cpp
	atf_check -s exit:0 -o inline:"hello world\n" ./hello
}

cxxruntime32_body() {
	# check whether this arch is 64bit
	if ! c++ -dM -E - < /dev/null | fgrep -q _LP64; then
		atf_skip "this is not a 64 bit architecture"
	fi
	if ! c++ -m32 -dM -E - < /dev/null 2>/dev/null > ./def32; then
		atf_skip "c++ -m32 not supported on this architecture"
	else
		if fgrep -q _LP64 ./def32; then
			atf_fail "c++ -m32 does not generate netbsd32 binaries"
		fi
	fi

	cat > test.cpp << EOF
#include <cstdlib>
#include <iostream>
int main(void) {std::cout << "hello world" << std::endl;exit(0);}
EOF
	atf_check -s exit:0 -o ignore -e ignore c++ -o hello32 -m32 test.cpp
	atf_check -s exit:0 -o ignore -e ignore c++ -o hello64 test.cpp
	file -b ./hello32 > ./ftype32
	file -b ./hello64 > ./ftype64
	if diff ./ftype32 ./ftype64 >/dev/null; then
		atf_fail "generated binaries do not differ"
	fi
	echo "32bit binaries on this platform are:"
	cat ./ftype32
	echo "While native (64bit) binaries are:"
	cat ./ftype64
	atf_check -s exit:0 -o inline:"hello world\n" ./hello32

	# do another test with static 32bit binaries
	cat > test.cpp << EOF
#include <cstdlib>
#include <iostream>
int main(void) {std::cout << "hello static world" << std::endl;exit(0);}
EOF
	atf_check -s exit:0 -o ignore -e ignore c++ -o hello -m32 \
	    -static test.cpp
	atf_check -s exit:0 -o inline:"hello static world\n" ./hello
}

atf_init_test_cases()
{
	atf_add_test_case cxxruntime
	atf_add_test_case cxxruntime_profile
	atf_add_test_case cxxruntime_pic
	atf_add_test_case cxxruntime_pie
	atf_add_test_case cxxruntime32
	atf_add_test_case cxxruntime_static
	atf_add_test_case cxxruntime_pic_32
	atf_add_test_case cxxruntime_pic_profile
	atf_add_test_case cxxruntime_pic_profile_32
	atf_add_test_case cxxruntime_profile_32
}
