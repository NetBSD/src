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

create_files()
{
    mkdir tmp
    touch tmp/datafile
}

atf_test_case default
default_head()
{
    atf_set "descr" "Checks that the program can find its files if" \
                    "executed from the same directory"
}
default_body()
{
    create_files

    for hp in $(get_helpers); do
        h=${hp##*/}
        cp ${hp} tmp
        atf_check "cd tmp && ./${h} srcdir_exists" 0 ignore ignore
        atf_check "${hp} srcdir_exists" 1 null stderr
        atf_check 'grep "Cannot.*find.*source.*directory" stderr' \
                  0 ignore null
    done
}

atf_test_case sflag
sflag_head()
{
    atf_set "descr" "Checks that the program can find its files when" \
                    "using the -s flag"
}
sflag_body()
{
    create_files

    for hp in $(get_helpers); do
        h=${hp##*/}
        cp ${hp} tmp
        atf_check "cd tmp && ./${h} -s $(pwd)/tmp \
                   srcdir_exists" 0 ignore ignore
        atf_check "${hp} srcdir_exists" 1 null stderr
        atf_check 'grep "Cannot.*find.*source.*directory" stderr' \
                  0 ignore null
        atf_check "${hp} -s $(pwd)/tmp srcdir_exists" 0 ignore ignore
    done
}

atf_test_case relative
relative_head()
{
    atf_set "descr" "Checks that passing a relative path through -s" \
                    "works"
}
relative_body()
{
    create_files

    for hp in $(get_helpers); do
        h=${hp##*/}
        cp ${hp} tmp

        for p in tmp tmp/. ./tmp; do
            echo "Helper is: ${h}"
            echo "Using source directory: ${p}"

            atf_check "./tmp/${h} -s ${p} \
                       srcdir_exists" 0 ignore ignore
            atf_check "${hp} srcdir_exists" 1 null stderr
            atf_check 'grep "Cannot.*find.*source.*directory" stderr' \
                      0 ignore null
            atf_check "${hp} -s ${p} srcdir_exists" 0 ignore ignore
        done
    done
}

atf_init_test_cases()
{
    atf_add_test_case default
    atf_add_test_case sflag
    atf_add_test_case relative
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
