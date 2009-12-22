#
# Automated Testing Framework (atf)
#
# Copyright (c) 2007, 2008, 2009 The NetBSD Foundation, Inc.
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

# TODO: This test program is probably incomplete.  It was first added
# to demonstrate the utility of the atf_check function.  Inspect its
# current status and complete it.

atf_test_case includes
includes_head()
{
    atf_set "descr" "Tests that the resulting file includes the correct" \
                    "shell subroutines"
    atf_set "require.progs" "atf-compile"
}
includes_body()
{
    cat >tp_test.sh <<EOF
# This is a sample test program.
EOF
    atf_check -s eq:0 -o empty -e empty atf-compile -o tp_test tp_test.sh
    atf_check -s eq:1 -o empty -e empty grep '^\..*/atf.init.subr' tp_test
    atf_check -s eq:0 -o ignore -e empty grep '^\..*/atf.header.subr' tp_test
    atf_check -s eq:0 -o ignore -e empty grep '^\..*/atf.footer.subr' tp_test
}

atf_test_case oflag
oflag_head()
{
    atf_set "descr" "Tests that the -o flag works"
    atf_set "require.progs" "atf-compile"
}
oflag_body()
{
    atf_check -s eq:0 -o empty -e empty touch tp_foo.sh
    atf_check -s eq:1 -o empty -e save:stderr atf-compile tp_foo.sh
    atf_check -s eq:0 -o ignore -e empty \
              grep "No output file specified" stderr

    atf_check -s eq:1 -o empty -e empty test -f tp_foo
    atf_check -s eq:0 -o empty -e empty atf-compile -o tp_foo tp_foo.sh
    atf_check -s eq:0 -o empty -e empty test -f tp_foo

    atf_check -s eq:1 -o empty -e empty test -f tp_foo2
    atf_check -s eq:0 -o empty -e empty atf-compile -o tp_foo2 tp_foo.sh
    atf_check -s eq:0 -o empty -e empty test -f tp_foo2

    atf_check -s eq:0 -o ignore -e empty cmp tp_foo tp_foo2
}

check_perms()
{
    srcdir=$(atf_get_srcdir)
    st_mode=$(${srcdir}/h_mode ${1})
    echo "File ${1}, st_mode ${st_mode}, umask ${3}, expecting ${2}"
    case ${st_mode} in
        *${2}) ;;
        *) atf_fail "File ${1} doesn't have permissions ${2}" ;;
    esac
}

atf_test_case perms
perms_head()
{
    atf_set "descr" "Tests the permissions of the generated file"
    atf_set "require.progs" "atf-compile"
}
perms_body()
{
    atf_check -s eq:0 -o empty -e empty touch tp_foo.sh

    atf_check -s eq:0 -o empty -e empty -x \
        'umask 0000 && atf-compile -o tp_foo tp_foo.sh'
    check_perms tp_foo 0777 0000
    rm -f tp_foo

    atf_check -s eq:0 -o empty -e empty -x \
        'umask 0002 && atf-compile -o tp_foo tp_foo.sh'
    check_perms tp_foo 0775 0002
    rm -f tp_foo

    atf_check -s eq:0 -o empty -e empty -x \
        'umask 0222 && atf-compile -o tp_foo tp_foo.sh'
    check_perms tp_foo 0555 0222
    rm -f tp_foo

    atf_check -s eq:0 -o empty -e empty -x \
        'umask 0777 && atf-compile -o tp_foo tp_foo.sh'
    check_perms tp_foo 0000 0777
    rm -f tp_foo
}

atf_init_test_cases()
{
    atf_add_test_case oflag
    atf_add_test_case includes
    atf_add_test_case perms
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
