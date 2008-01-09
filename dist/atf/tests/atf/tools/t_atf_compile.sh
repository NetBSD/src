#
# Automated Testing Framework (atf)
#
# Copyright (c) 2007 The NetBSD Foundation, Inc.
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
    atf_check 'atf-compile -o tp_test tp_test.sh' 0 null null
    atf_check 'grep ^\..*/atf.init.subr tp_test' 1 null null
    atf_check 'grep ^\..*/atf.header.subr tp_test' 0 ignore null
    atf_check 'grep ^\..*/atf.footer.subr tp_test' 0 ignore null
}

atf_test_case oflag
oflag_head()
{
    atf_set "descr" "Tests that the -o flag works"
    atf_set "require.progs" "atf-compile"
}
oflag_body()
{
    atf_check 'touch tp_foo.sh' 0 null null
    atf_check 'atf-compile tp_foo.sh' 1 null stderr
    atf_check 'grep "No output file specified" stderr' 0 ignore null

    atf_check 'test -f tp_foo' 1 null null
    atf_check 'atf-compile -o tp_foo tp_foo.sh' 0 null null
    atf_check 'test -f tp_foo' 0 null null

    atf_check 'test -f tp_foo2' 1 null null
    atf_check 'atf-compile -o tp_foo2 tp_foo.sh' 0 null null
    atf_check 'test -f tp_foo2' 0 null null

    atf_check 'cmp tp_foo tp_foo2' 0 ignore null
}

check_perms()
{
    srcdir=$(atf_get_srcdir)
    st_mode=$(${srcdir}/h_mode ${1})
    echo "File ${1}, st_mode ${st_mode}, umask $(umask), expecting ${2}"
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
    atf_check 'touch tp_foo.sh' 0 null null

    umask 0000
    atf_check 'atf-compile -o tp_foo tp_foo.sh' 0 null null
    check_perms tp_foo 0777
    rm -f tp_foo

    umask 0002
    atf_check 'atf-compile -o tp_foo tp_foo.sh' 0 null null
    check_perms tp_foo 0775
    rm -f tp_foo

    umask 0222
    atf_check 'atf-compile -o tp_foo tp_foo.sh' 0 null null
    check_perms tp_foo 0555
    rm -f tp_foo

    umask 0777
    atf_check 'atf-compile -o tp_foo tp_foo.sh' 0 null null
    check_perms tp_foo 0000
    rm -f tp_foo
}

atf_init_test_cases()
{
    atf_add_test_case oflag
    atf_add_test_case includes
    atf_add_test_case perms
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
