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

atf_test_case default
default_head()
{
    atf_set "descr" "Tests that the default work directory is used" \
                    "correctly"
}
default_body()
{
    tmpdir=$(cd $(atf-config -t atf_workdir) && pwd -P)

    for h in $(get_helpers); do
        atf_check "${h} -s $(atf_get_srcdir) \
                  -v pathfile=$(pwd)/path workdir_path" 0 ignore ignore
        atf_check "grep '^${tmpdir}/atf.' <path" 0 ignore ignore
    done
}

atf_test_case tmpdir
tmpdir_head()
{
    atf_set "descr" "Tests that the work directory is used correctly when" \
                    "overridden through the ATF_WORKDIR environment" \
                    "variable"
}
tmpdir_body()
{
    tmpdir=$(pwd -P)/workdir
    mkdir ${tmpdir}

    for h in $(get_helpers); do
        atf_check "ATF_WORKDIR=${tmpdir} ${h} -s $(atf_get_srcdir) \
                   -v pathfile=$(pwd)/path workdir_path" 0 ignore ignore
        atf_check "grep '^${tmpdir}/atf.' <path" 0 ignore ignore
    done
}

atf_test_case conf
conf_head()
{
    atf_set "descr" "Tests that the work directory is used correctly when" \
                    "overridden through the test program's -w flag"
}
conf_body()
{
    tmpdir=$(pwd -P)/workdir
    mkdir ${tmpdir}

    for h in $(get_helpers); do
        atf_check "${h} -s $(atf_get_srcdir) \
                   -v pathfile=$(pwd)/path -w ${tmpdir} \
                   workdir_path" 0 ignore ignore
        atf_check "grep '^${tmpdir}/atf.' <path" 0 ignore null
    done
}

atf_test_case cleanup
cleanup_head()
{
    atf_set "descr" "Tests that the directories used to isolate test" \
                    "cases are properly cleaned up once the test case" \
                    "exits"
}
cleanup_body()
{
    tmpdir=$(pwd -P)/workdir
    mkdir ${tmpdir}

    for h in $(get_helpers); do
        # First try to clean a work directory that, supposedly, does not
        # have any subdirectories.
        atf_check "${h} -s $(atf_get_srcdir) \
                   -v pathfile=$(pwd)/path -w ${tmpdir} \
                   workdir_path" 0 ignore ignore
        atf_check "test -d $(cat path)" 1 null null
        set -- ${tmpdir}/atf.*
        [ -f "${1}" ] && \
            atf_fail "Test program did not properly remove all temporary" \
                     "files"

        # Now do the same but with a work directory that has subdirectories.
        # The program will have to recurse into them to clean them all.
        atf_check "${h} -s $(atf_get_srcdir) -v pathfile=$(pwd)/path \
                   -w ${tmpdir} workdir_cleanup" 0 ignore ignore
        atf_check "test -d $(cat path)" 1 null null
    done
}

atf_test_case missing
missing_head()
{
    atf_set "descr" "Tests that an error is raised if the work directory" \
                    "does not exist"
}
missing_body()
{
    tmpdir=$(pwd -P)/workdir

    for h in $(get_helpers); do
        atf_check "${h} -s $(atf_get_srcdir) \
                   -v pathfile=$(pwd)/path -w ${tmpdir} \
                   workdir_path" 1 null stderr
        atf_check "grep 'Cannot find.*${tmpdir}' stderr" 0 ignore null
    done
}

# -------------------------------------------------------------------------
# Main.
# -------------------------------------------------------------------------

atf_init_test_cases()
{
    atf_add_test_case default
    atf_add_test_case tmpdir
    atf_add_test_case conf
    atf_add_test_case cleanup
    atf_add_test_case missing
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
