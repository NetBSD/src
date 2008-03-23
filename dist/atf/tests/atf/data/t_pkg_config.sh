#
# Automated Testing Framework (atf)
#
# Copyright (c) 2008 The NetBSD Foundation, Inc.
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
# 3. All advertising materials mentioning features or use of this
#    software must display the following acknowledgement:
#        This product includes software developed by the NetBSD
#        Foundation, Inc. and its contributors.
# 4. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
# CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# The following tests assume that the atf.pc file is installed in a
# directory that is known by pkg-config.  Otherwise they will fail,
# and you will be required to adjust PKG_CONFIG_PATH accordingly.
#
# It would be possible to bypass this requirement by setting the path
# explicitly during the tests, but then this would not do a real check
# to ensure that the installation is working.

require_atf_pc()
{
    pkg-config atf || atf_fail "pkg-config could not locate atf.pc;" \
                               "maybe need to set PKG_CONFIG_PATH?"
}

atf_test_case version
version_head()
{
    atf_set "descr" "Checks that the version is correct"
    atf_set "require.progs" "pkg-config"
}
version_body()
{
    require_atf_pc

    atf_check "atf-version | head -n 1 | cut -d ' ' -f 4" 0 stdout null
    ver1=$(cat stdout)
    echo "Version reported by atf-version: ${ver1}"

    atf_check "pkg-config --modversion atf" 0 stdout null
    ver2=$(cat stdout)
    echo "Version reported by pkg-config: ${ver2}"

    atf_check_equal ${ver1} ${ver2}
}

atf_test_case build
build_head()
{
    atf_set "descr" "Checks that a test program can be built against" \
                    "the C++ library based on the pkg-config information"
    atf_set "require.progs" "pkg-config"
}
build_body()
{
    require_atf_pc

    atf_check "pkg-config --variable=cxx atf" 0 stdout null
    cxx=$(cat stdout)
    echo "Compiler is: ${cxx}"
    atf_require_prog ${cxx}

    cat >tp.cpp <<EOF
#include <iostream>

#include <atf.hpp>

ATF_TEST_CASE(tp);
ATF_TEST_CASE_HEAD(tp) {
    set("descr", "A test case");
}
ATF_TEST_CASE_BODY(tp) {
    std::cout << "Running" << std::endl;
}

ATF_INIT_TEST_CASES(tcs) {
    ATF_ADD_TEST_CASE(tcs, tp);
}
EOF

    atf_check "pkg-config --cflags atf" 0 stdout null
    cxxflags=$(cat stdout)
    echo "CXXFLAGS are: ${cxxflags}"

    atf_check "pkg-config --libs-only-L --libs-only-other atf" 0 stdout null
    ldflags=$(cat stdout)
    atf_check "pkg-config --libs-only-l atf" 0 stdout null
    libs=$(cat stdout)
    echo "LDFLAGS are: ${ldflags}"
    echo "LIBS are: ${libs}"

    atf_check "${cxx} ${cxxflags} -o tp.o -c tp.cpp" 0 null null
    atf_check "${cxx} ${ldflags} -o tp tp.o ${libs}" 0 null null

    libpath=
    for f in ${ldflags}; do
        case ${f} in
            -L*)
                dir=$(echo ${f} | sed -e 's,^-L,,')
                if [ -z "${libpath}" ]; then
                    libpath="${dir}"
                else
                    libpath="${libpath}:${dir}"
                fi
                ;;
            *)
                ;;
        esac
    done

    atf_check "test -x tp" 0 null null
    atf_check "LD_LIBRARY_PATH=${libpath} ./tp" 0 stdout null
    atf_check "grep 'application/X-atf-tcs' stdout" 0 ignore null
    atf_check "grep 'Running' stdout" 0 ignore null
}

atf_init_test_cases()
{
    atf_add_test_case version
    atf_add_test_case build
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
