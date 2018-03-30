#	$NetBSD: t_call_once2.sh,v 1.1.2.2 2018/03/30 06:20:16 pgoyette Exp $
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

atf_test_case call_once2
call_once2_head() {
	atf_set "descr" "compile and run std::call_once"
	atf_set "require.progs" "c++"
}

atf_test_case call_once2_profile
call_once2_profile_head() {
	atf_set "descr" "compile and run std::call_once with profiling option"
	atf_set "require.progs" "c++"
}

atf_test_case call_once2_pic
call_once2_pic_head() {
	atf_set "descr" "compile and run PIC std::call_once"
	atf_set "require.progs" "c++"
}

atf_test_case call_once2_pic_32
call_once2_pic_32_head() {
	atf_set "descr" "compile and run 32-bit PIC std::call_once"
	atf_set "require.progs" "c++"
}

atf_test_case call_once2_pic_profile
call_once2_pic_head() {
	atf_set "descr" "compile and run PIC std::call_once with profiling flag"
	atf_set "require.progs" "c++"
}

atf_test_case call_once2_pic_profile_32
call_once2_pic_profile_32_head() {
	atf_set "descr" "compile and run 32-bit PIC std::call_once with profiling flag"
	atf_set "require.progs" "c++"
}

atf_test_case call_once2_profile_32
call_once2_profile_32_head() {
	atf_set "descr" "compile and run 32-bit std::call_once with profiling flag"
	atf_set "require.progs" "c++"
}

atf_test_case call_once2_pie
call_once2_pie_head() {
	atf_set "descr" "compile and run position independent (PIE) std::call_once"
	atf_set "require.progs" "c++"
}

atf_test_case call_once2_32
call_once2_32_head() {
	atf_set "descr" "compile and run std::call_once for/in netbsd32 emulation"
	atf_set "require.progs" "c++ file diff cat"
}

atf_test_case call_once2_static
call_once2_static_head() {
	atf_set "descr" "compile and run std::call_once with static flag"
	atf_set "require.progs" "c++"
}

call_once2_body() {
	cat > test.cpp << EOF
#include <mutex>
#include <thread>
#include <iostream>
std::once_flag flag, flag_throw;
void print_once(void) { std::call_once(flag, [](){ std::cout << "hello, " << std::flush; }); }
void throw_once(void) { throw std::exception(); }
int main(void) {
        static const int nr_threads(4);
        std::thread threads[nr_threads];

        for (int i = 0; i < nr_threads; ++i) {
                threads[i] = std::thread(print_once);
        }
        for (int i = 0; i < nr_threads; ++i) {
                threads[i].join();
        }

        try {
                std::call_once(flag_throw, throw_once);
        } catch (...) {
                std::cout << "world!" << std::endl;
        }
        return 0;
}
EOF
	atf_check -s exit:0 -o ignore -e ignore c++ -o call_once2 test.cpp -pthread
	atf_check -s exit:0 -o inline:"hello, world!\n" ./call_once2
}

call_once2_profile_body() {
	atf_expect_fail "profiling option doesn't work now"
	cat > test.cpp << EOF
#include <mutex>
#include <thread>
#include <iostream>
std::once_flag flag, flag_throw;
void print_once(void) { std::call_once(flag, [](){ std::cout << "hello, " << std::flush; }); }
void throw_once(void) { throw std::exception(); }
int main(void) {
        static const int nr_threads(4);
        std::thread threads[nr_threads];

        for (int i = 0; i < nr_threads; ++i) {
                threads[i] = std::thread(print_once);
        }
        for (int i = 0; i < nr_threads; ++i) {
                threads[i].join();
        }

        try {
                std::call_once(flag_throw, throw_once);
        } catch (...) {
                std::cout << "world!" << std::endl;
        }
        return 0;
}
EOF
	atf_check -s exit:0 -o ignore -e ignore c++ -pg -o call_once2 test.cpp -pthread
	atf_check -s exit:0 -o inline:"hello, world!\n" ./call_once2
}

call_once2_profile_32_body() {
	atf_expect_fail "profiling option doesn't work now"
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
#include <mutex>
#include <thread>
#include <iostream>
std::once_flag flag, flag_throw;
void print_once(void) { std::call_once(flag, [](){ std::cout << "hello, " << std::flush; }); }
void throw_once(void) { throw std::exception(); }
int main(void) {
        static const int nr_threads(4);
        std::thread threads[nr_threads];

        for (int i = 0; i < nr_threads; ++i) {
                threads[i] = std::thread(print_once);
        }
        for (int i = 0; i < nr_threads; ++i) {
                threads[i].join();
        }

        try {
                std::call_once(flag_throw, throw_once);
        } catch (...) {
                std::cout << "world!" << std::endl;
        }
        return 0;
}
EOF
	atf_check -s exit:0 -o ignore -e ignore c++ -m32 -pg -o call_once2 test.cpp -pthread
	atf_check -s exit:0 -o inline:"hello, world!\n" ./call_once2
	atf_expect_fail "The combination of 32-bit and profiling should be fail"
}

call_once2_pic_body() {
	cat > test.cpp << EOF
#include <stdlib.h>
int callpic(void);
int main(void) {callpic();exit(0);}
EOF
	cat > pic.cpp << EOF
#include <mutex>
#include <thread>
#include <iostream>
std::once_flag flag, flag_throw;
void print_once(void) { std::call_once(flag, [](){ std::cout << "hello, " << std::flush; }); }
void throw_once(void) { throw std::exception(); }
int callpic(void) {
        static const int nr_threads(4);
        std::thread threads[nr_threads];

        for (int i = 0; i < nr_threads; ++i) {
                threads[i] = std::thread(print_once);
        }
        for (int i = 0; i < nr_threads; ++i) {
                threads[i].join();
        }

        try {
                std::call_once(flag_throw, throw_once);
        } catch (...) {
                std::cout << "world!" << std::endl;
        }
        return 0;
}
EOF

	atf_check -s exit:0 -o ignore -e ignore \
	    c++ -fPIC -shared -o libtest.so pic.cpp
	atf_check -s exit:0 -o ignore -e ignore \
	    c++ -o call_once2 test.cpp -L. -ltest -pthread

	export LD_LIBRARY_PATH=.
	atf_check -s exit:0 -o inline:"hello, world!\n" ./call_once2
}

call_once2_pic_32_body() {
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
#include <stdlib.h>
int callpic(void);
int main(void) {callpic();exit(0);}
EOF
	cat > pic.cpp << EOF
#include <mutex>
#include <thread>
#include <iostream>
std::once_flag flag, flag_throw;
void print_once(void) { std::call_once(flag, [](){ std::cout << "hello, " << std::flush; }); }
void throw_once(void) { throw std::exception(); }
int callpic(void) {
        static const int nr_threads(4);
        std::thread threads[nr_threads];

        for (int i = 0; i < nr_threads; ++i) {
                threads[i] = std::thread(print_once);
        }
        for (int i = 0; i < nr_threads; ++i) {
                threads[i].join();
        }

        try {
                std::call_once(flag_throw, throw_once);
        } catch (...) {
                std::cout << "world!" << std::endl;
        }
        return 0;
}
EOF

	atf_check -s exit:0 -o ignore -e ignore \
	    c++ -m32 -fPIC -shared -o libtest.so pic.cpp
	atf_check -s exit:0 -o ignore -e ignore \
	    c++ -m32 -o call_once2 test.cpp -L. -ltest -pthread

	export LD_LIBRARY_PATH=.
	atf_check -s exit:0 -o inline:"hello, world!\n" ./call_once2
}

call_once2_pic_profile_body() {
	atf_expect_fail "profiling option doesn't work now"
	cat > test.cpp << EOF
#include <stdlib.h>
int callpic(void);
int main(void) {callpic();exit(0);}
EOF
	cat > pic.cpp << EOF
#include <mutex>
#include <thread>
#include <iostream>
std::once_flag flag, flag_throw;
void print_once(void) { std::call_once(flag, [](){ std::cout << "hello, " << std::flush; }); }
void throw_once(void) { throw std::exception(); }
int callpic(void) {
        static const int nr_threads(4);
        std::thread threads[nr_threads];

        for (int i = 0; i < nr_threads; ++i) {
                threads[i] = std::thread(print_once);
        }
        for (int i = 0; i < nr_threads; ++i) {
                threads[i].join();
        }

        try {
                std::call_once(flag_throw, throw_once);
        } catch (...) {
                std::cout << "world!" << std::endl;
        }
        return 0;
}
EOF

	atf_check -s exit:0 -o ignore -e ignore \
	    c++ -pg -fPIC -shared -o libtest.so pic.cpp
	atf_check -s exit:0 -o ignore -e ignore \
	    c++ -pg -o call_once2 test.cpp -L. -ltest -pthread

	export LD_LIBRARY_PATH=.
	atf_check -s exit:0 -o inline:"hello, world!\n" ./call_once2
}

call_once2_pic_profile_32_body() {
	atf_expect_fail "profiling option doesn't work now"
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
#include <stdlib.h>
int callpic(void);
int main(void) {callpic();exit(0);}
EOF
	cat > pic.cpp << EOF
#include <mutex>
#include <thread>
#include <iostream>
std::once_flag flag, flag_throw;
void print_once(void) { std::call_once(flag, [](){ std::cout << "hello, " << std::flush; }); }
void throw_once(void) { throw std::exception(); }
int callpic(void) {
        static const int nr_threads(4);
        std::thread threads[nr_threads];

        for (int i = 0; i < nr_threads; ++i) {
                threads[i] = std::thread(print_once);
        }
        for (int i = 0; i < nr_threads; ++i) {
                threads[i].join();
        }

        try {
                std::call_once(flag_throw, throw_once);
        } catch (...) {
                std::cout << "world!" << std::endl;
        }
        return 0;
}
EOF

	atf_check -s exit:0 -o ignore -e ignore \
	    c++ -m32 -pg -fPIC -shared -o libtest.so pic.cpp
	atf_check -s exit:0 -o ignore -e ignore \
	    c++ -m32 -pg -o call_once2 test.cpp -L. -ltest -pthread

	export LD_LIBRARY_PATH=.
	atf_check -s exit:0 -o inline:"hello, world!\n" ./call_once2
}

call_once2_pie_body() {
	# check whether this arch supports -pie
	if ! c++ -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then
		atf_skip "c++ -pie not supported on this architecture"
	fi
	cat > test.cpp << EOF
#include <mutex>
#include <thread>
#include <iostream>
std::once_flag flag, flag_throw;
void print_once(void) { std::call_once(flag, [](){ std::cout << "hello, " << std::flush; }); }
void throw_once(void) { throw std::exception(); }
int main(void) {
        static const int nr_threads(4);
        std::thread threads[nr_threads];

        for (int i = 0; i < nr_threads; ++i) {
                threads[i] = std::thread(print_once);
        }
        for (int i = 0; i < nr_threads; ++i) {
                threads[i].join();
        }

        try {
                std::call_once(flag_throw, throw_once);
        } catch (...) {
                std::cout << "world!" << std::endl;
        }
        return 0;
}
EOF
	atf_check -s exit:0 -o ignore -e ignore c++ -fpie -pie -o call_once2 test.cpp -pthread
	atf_check -s exit:0 -o inline:"hello, world!\n" ./call_once2
}

call_once2_32_body() {
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
#include <mutex>
#include <thread>
#include <iostream>
std::once_flag flag, flag_throw;
void print_once(void) { std::call_once(flag, [](){ std::cout << "hello, " << std::flush; }); }
void throw_once(void) { throw std::exception(); }
int main(void) {
        static const int nr_threads(4);
        std::thread threads[nr_threads];

        for (int i = 0; i < nr_threads; ++i) {
                threads[i] = std::thread(print_once);
        }
        for (int i = 0; i < nr_threads; ++i) {
                threads[i].join();
        }

        try {
                std::call_once(flag_throw, throw_once);
        } catch (...) {
                std::cout << "world!" << std::endl;
        }
        return 0;
}
EOF
	atf_check -s exit:0 -o ignore -e ignore c++ -o call_once2_32 -m32 test.cpp -pthread
	atf_check -s exit:0 -o ignore -e ignore c++ -o call_once2_64 test.cpp -pthread
	file -b ./call_once2_32 > ./ftype32
	file -b ./call_once2_64 > ./ftype64
	if diff ./ftype32 ./ftype64 >/dev/null; then
		atf_fail "generated binaries do not differ"
	fi
	echo "32bit binaries on this platform are:"
	cat ./ftype32
	echo "While native (64bit) binaries are:"
	cat ./ftype64
	atf_check -s exit:0 -o inline:"hello, world!\n" ./call_once2_32

	# do another test with static 32bit binaries
	cat > test.cpp << EOF
#include <mutex>
#include <thread>
#include <iostream>
std::once_flag flag, flag_throw;
void print_once(void) { std::call_once(flag, [](){ std::cout << "hello, " << std::flush; }); }
void throw_once(void) { throw std::exception(); }
int main(void) {
        static const int nr_threads(4);
        std::thread threads[nr_threads];

        for (int i = 0; i < nr_threads; ++i) {
                threads[i] = std::thread(print_once);
        }
        for (int i = 0; i < nr_threads; ++i) {
                threads[i].join();
        }

        try {
                std::call_once(flag_throw, throw_once);
        } catch (...) {
                std::cout << "world!" << std::endl;
        }
        return 0;
}
EOF
	atf_check -s exit:0 -o ignore -e ignore c++ -o call_once2 -m32 -pthread \
	    -static test.cpp
	atf_check -s exit:0 -o inline:"hello, world!\n" ./call_once2
}

call_once2_static_body() {
	cat > test.cpp << EOF
#include <mutex>
#include <thread>
#include <iostream>
std::once_flag flag, flag_throw;
void print_once(void) { std::call_once(flag, [](){ std::cout << "hello, " << std::flush; }); }
void throw_once(void) { throw std::exception(); }
int main(void) {
        static const int nr_threads(4);
        std::thread threads[nr_threads];

        for (int i = 0; i < nr_threads; ++i) {
                threads[i] = std::thread(print_once);
        }
        for (int i = 0; i < nr_threads; ++i) {
                threads[i].join();
        }

        try {
                std::call_once(flag_throw, throw_once);
        } catch (...) {
                std::cout << "world!" << std::endl;
        }
        return 0;
}
EOF
	atf_check -s exit:0 -o ignore -e ignore c++ -static -o call_once2 test.cpp -pthread
	atf_check -s exit:0 -o inline:"hello, world!\n" ./call_once2
}

atf_init_test_cases()
{

	atf_add_test_case call_once2
	atf_add_test_case call_once2_profile
	atf_add_test_case call_once2_pic
	atf_add_test_case call_once2_pie
	atf_add_test_case call_once2_32
	atf_add_test_case call_once2_static
	atf_add_test_case call_once2_pic_32
	atf_add_test_case call_once2_pic_profile
	atf_add_test_case call_once2_pic_profile_32
	atf_add_test_case call_once2_profile_32
}
