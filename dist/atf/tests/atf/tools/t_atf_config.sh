#
# Automated Testing Framework (atf)
#
# Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
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

all_vars="atf_arch atf_confdir atf_libexecdir atf_machine atf_pkgdatadir \
          atf_shell atf_workdir"
all_vars_no=7

count_lines()
{
    test $(wc -l $1 | awk '{ print $1 }') = $2
}

atf_test_case list_all
list_all_head()
{
    atf_set "descr" "Tests that at atf-config prints all expected" \
                    "variables, and not more"
}
list_all_body()
{
    atf_check "atf-config" 0 stdout null
    atf_check "count_lines stdout ${all_vars_no}" 0 null null
    for v in ${all_vars}; do
        atf_check "grep ${v} stdout" 0 ignore null
    done
}

atf_test_case query_one
query_one_head()
{
    atf_set "descr" "Tests that querying a single variable works"
}
query_one_body()
{
    for v in ${all_vars}; do
        atf_check "atf-config ${v}" 0 stdout null
        atf_check "count_lines stdout 1" 0 null null
        atf_check "grep ${v} stdout" 0 ignore null
    done
}

atf_test_case query_one_terse
query_one_terse_head()
{
    atf_set "descr" "Tests that querying a single variable in terse mode" \
                    "works"
}
query_one_terse_body()
{
    for v in ${all_vars}; do
        atf_check "atf-config ${v}" 0 stdout null
        atf_check "count_lines stdout 1" 0 null null
        atf_check "grep ${v} stdout" 0 ignore null
        atf_check "awk '{ print \$3 }' stdout" 0 stdout null
        atf_check "mv stdout expout" 0 null null
        atf_check "atf-config -t ${v}" 0 expout null
    done
}

atf_test_case query_multiple
query_multiple_head()
{
    atf_set "descr" "Tests that querying multiple variables works"
}
query_multiple_body()
{
    atf_check "atf-config atf_libexecdir atf_shell" 0 stdout null
    atf_check "count_lines stdout 2" 0 null null
    atf_check "grep atf_libexecdir stdout" 0 ignore null
    atf_check "grep atf_shell stdout" 0 ignore null
}

atf_test_case query_unknown
query_unknown_head()
{
    atf_set "descr" "Tests that querying an unknown variable delivers" \
                    "the correct error"
}
query_unknown_body()
{
    atf_check 'atf-config non_existent' 1 null stderr
    atf_check 'grep "Unknown variable.*non_existent" stderr' 0 ignore null
}

atf_test_case query_mixture
query_mixture_head()
{
    atf_set "descr" "Tests that querying a known and an unknown variable" \
                    "delivers the correct error"
}
query_mixture_body()
{
    for v in ${all_vars}; do
        atf_check "atf-config ${v} non_existent" 1 null stderr
        atf_check "grep 'Unknown variable.*non_existent' stderr" 0 ignore null
        atf_check "atf-config non_existent ${v}" 1 null stderr
        atf_check "grep 'Unknown variable.*non_existent' stderr" 0 ignore null
    done
}

atf_test_case override_env
override_env_head()
{
    atf_set "descr" "Tests that build-time variables can be overriden" \
                    "through their corresponding environment variables"
}
override_env_body()
{
    for v in ${all_vars}; do
        V=$(echo ${v} | tr '[a-z]' '[A-Z]')
        atf_check "${V}=testval atf-config" 0 stdout null
        atf_check "mv stdout all" 0 null null

        atf_check "grep '${v}' all" 0 stdout null
        atf_check "mv stdout affected" 0 null null
        atf_check "grep -v '${v}' all" 0 stdout null
        atf_check "mv stdout unaffected" 0 null null

        atf_check "count_lines affected 1" 0 null null
        atf_check "count_lines unaffected $((${all_vars_no} - 1))" 0 null null

        atf_check "grep '${v} : testval' affected" 0 ignore null
        atf_check "grep 'testval' unaffected" 1 null null
    done
}

# XXX: This does not seem to belong here...
atf_test_case arch
arch_head()
{
    atf_set "descr" "Tests that the current value of atf_arch is correct" \
                    "for the corresponding atf_machine"
}
arch_body()
{
    atf_check "atf-config -t atf_arch" 0 stdout null
    arch=$(cat stdout)
    atf_check "atf-config -t atf_machine" 0 stdout null
    machine=$(cat stdout)
    echo "Machine type ${machine}, architecture ${arch}"

    case ${machine} in
        i386|i486|i586|i686)
            exp_arch=i386
            ;;
        *)
            exp_arch=${machine}
    esac
    echo "Expected architecture ${exp_arch}"

    atf_check_equal ${arch} ${exp_arch}
}

atf_init_test_cases()
{
    atf_add_test_case list_all

    atf_add_test_case query_one
    atf_add_test_case query_one_terse
    atf_add_test_case query_multiple
    atf_add_test_case query_unknown
    atf_add_test_case query_mixture

    atf_add_test_case override_env

    atf_add_test_case arch
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
