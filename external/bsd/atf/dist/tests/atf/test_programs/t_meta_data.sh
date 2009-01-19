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

# -------------------------------------------------------------------------
# Tests for the "ident" meta-data property.
# -------------------------------------------------------------------------

atf_test_case ident
ident_head()
{
    atf_set "descr" "Tests that the ident property works"
}
ident_body()
{
    for h in $(get_helpers); do
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 ident_1 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep passed resout
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 ident_2 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep passed resout
    done
}

# -------------------------------------------------------------------------
# Tests for the "require_arch" meta-data property.
# -------------------------------------------------------------------------

atf_test_case require_arch
require_arch_head()
{
    atf_set "descr" "Tests that the require.arch property works"
}
require_arch_body()
{
    for h in $(get_helpers); do
        echo "Check for the real architecture"
        arch=$(atf-config -t atf_arch)
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v arch='${arch}' \
                   require_arch 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v arch='foo ${arch}' \
                   require_arch 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v arch='${arch} foo' \
                   require_arch 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout

        echo "Some fictitious checks"
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "ATF_ARCH=foo ${h} -s $(atf_get_srcdir) -r3 \
                   -v arch='foo' \
                   require_arch 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "ATF_ARCH=foo ${h} -s $(atf_get_srcdir) -r3 \
                   -v arch='foo bar' \
                   require_arch 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "ATF_ARCH=foo ${h} -s $(atf_get_srcdir) -r3 \
                   -v arch='bar foo' \
                   require_arch 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout

        echo "Now some failures"
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "ATF_ARCH=foo ${h} -s $(atf_get_srcdir) -r3 \
                   -v arch='bar' \
                   require_arch 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "skipped" resout
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "ATF_ARCH=foo ${h} -s $(atf_get_srcdir) -r3 \
                   -v arch='bar baz' \
                   require_arch 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "skipped" resout
    done
}

# -------------------------------------------------------------------------
# Tests for the "require_config" meta-data property.
# -------------------------------------------------------------------------

atf_test_case require_config
require_config_head()
{
    atf_set "descr" "Tests that the require.config property works"
}
require_config_body()
{
    for h in $(get_helpers); do
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 require_config 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "skipped" resout
        atf_check -s eq:0 -o ignore -e empty grep "var1 not defined" resout

        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v var1=foo \
                   require_config 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "skipped" resout
        atf_check -s eq:0 -o ignore -e empty grep "var2 not defined" resout

        atf_check -s eq:0 -o save:stdout -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v var1=foo -v var2=bar \
                   require_config 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout
        atf_check -s eq:0 -o ignore -e empty grep "var1: foo" stdout
        atf_check -s eq:0 -o ignore -e empty grep "var2: bar" stdout
    done
}

# -------------------------------------------------------------------------
# Tests for the "require_machine" meta-data property.
# -------------------------------------------------------------------------

atf_test_case require_machine
require_machine_head()
{
    atf_set "descr" "Tests that the require.machine property works"
}
require_machine_body()
{
    for h in $(get_helpers); do
        echo "Check for the real machine type"
        machine=$(atf-config -t atf_machine)
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v machine='${machine}' \
                   require_machine 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v machine='foo ${machine}' \
                   require_machine 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v machine='${machine} foo' \
                   require_machine 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout

        echo "Some fictitious checks"
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "ATF_MACHINE=foo ${h} -s $(atf_get_srcdir) -r3 \
                   -v machine='foo' \
                   require_machine 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "ATF_MACHINE=foo ${h} -s $(atf_get_srcdir) -r3 \
                   -v machine='foo bar' \
                   require_machine 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "ATF_MACHINE=foo ${h} -s $(atf_get_srcdir) -r3 \
                   -v machine='bar foo' \
                   require_machine 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout

        echo "Now some failures"
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "ATF_MACHINE=foo ${h} -s $(atf_get_srcdir) -r3 \
                   -v machine='bar' \
                   require_machine 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "skipped" resout
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "ATF_MACHINE=foo ${h} -s $(atf_get_srcdir) -r3 \
                   -v machine='bar baz' \
                   require_machine 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "skipped" resout
    done
}

# -------------------------------------------------------------------------
# Tests for the "require_progs" meta-data property.
# -------------------------------------------------------------------------

common_tests() {
    where=${1}
    for h in $(get_helpers); do
        # Check absolute paths.
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v 'progs=/bin/cp' \
                   require_progs_${where} 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v \
                   'progs=/bin/__non-existent__' \
                   require_progs_${where} 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "skipped" resout

        # Relative paths are not allowed.
        atf_check -s eq:1 -o ignore -e save:stderr -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v 'progs=bin/cp' \
                   require_progs_${where} 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "failed" resout

        # Check plain file names, searching them in the PATH.
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v 'progs=cp' \
                   require_progs_${where} 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v 'progs=__non-existent__' \
                   require_progs_${where} 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "skipped" resout
    done
}

# XXX This does not belong here.  This is a unit test.
atf_test_case require_progs_func
require_progs_func_head()
{
    atf_set "descr" "Tests that atf_require_prog works"
}
require_progs_func_body()
{
    common_tests body
}

atf_test_case require_progs_header
require_progs_header_head()
{
    atf_set "descr" "Tests that require.progs works"
}
require_progs_header_body()
{
    common_tests head

    for h in $(get_helpers); do
        # Check a couple of absolute path names.  The second must make
        # the check fail.
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 \
                   -v 'progs=/bin/cp /bin/__non-existent__' \
                   require_progs_head 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "skipped" resout
        atf_check -s eq:0 -o ignore -e empty grep "non-existent" resout

        # Check a couple of absolute path names.  Both have to be found.
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v 'progs=/bin/cp /bin/ls' \
                   require_progs_head 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout

        # Check an absolute path name and a relative one.  The second must
        # make the check fail.
        atf_check -s eq:1 -o ignore -e save:stderr -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v 'progs=/bin/cp bin/cp' \
                   require_progs_head 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "failed" resout

        # Check an absolute path name and a plain one.  Both have to be
        # found.
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v 'progs=/bin/cp ls' \
                   require_progs_head 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "passed" resout

        # Check an absolute path name and a plain one.  The second must
        # make the check fail.
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 \
                   -v 'progs=/bin/cp __non-existent__' require_progs_head \
                   3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "skipped" resout
        atf_check -s eq:0 -o ignore -e empty grep "non-existent" resout
    done
}

# -------------------------------------------------------------------------
# Tests for the "require_user" meta-data property.
# -------------------------------------------------------------------------

count_lines()
{
    wc -l $1 | awk '{ print $1 }'
}

atf_test_case require_user_root
require_user_root_head()
{
    atf_set "descr" "Tests that 'require.user=root' works"
}
require_user_root_body()
{
    for h in $(get_helpers); do
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v user=root require_user \
                   3>resout"
        if [ $(id -u) -eq 0 ]; then
            atf_check -s eq:0 -o ignore -e empty grep "passed" resout
        else
            atf_check -s eq:0 -o ignore -e empty grep "skipped" resout
        fi
    done
}

atf_test_case require_user_unprivileged
require_user_unprivileged_head()
{
    atf_set "descr" "Tests that 'require.user=unprivileged' works"
}
require_user_unprivileged_body()
{
    for h in $(get_helpers); do
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v user=unprivileged \
                   require_user 3>resout"
        if [ $(id -u) -eq 0 ]; then
            atf_check -s eq:0 -o ignore -e empty grep "skipped" resout
        else
            atf_check -s eq:0 -o ignore -e empty grep "passed" resout
        fi
    done
}

atf_test_case require_user_multiple
require_user_multiple_head()
{
    atf_set "descr" "Tests that multiple skip results raised by the" \
                    "handling of require.user do not abort the program" \
                    "prematurely"
}
require_user_multiple_body()
{
    for h in $(get_helpers); do
        if [ $(id -u) -eq 0 ]; then
            users="-v user=unprivileged -v user2=unprivileged -v user3=root"
        else
            users="-v user=root -v user2=root -v user3=unprivileged"
        fi
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 ${users} require_user \
                   require_user2 require_user3 3>resout"
        grep "skipped" resout >skips
        [ $(count_lines skips) -lt 2 ] && \
            atf_fail "Test program aborted prematurely"
        [ $(count_lines skips) -gt 2 ] && \
            atf_fail "Test program returned more skips than expected"
    done
}

atf_test_case require_user_bad
require_user_bad_head()
{
    atf_set "descr" "Tests that passing an invalid value to require.user" \
                    "raises an error"
}
require_user_bad_body()
{
    for h in $(get_helpers); do
        atf_check -s eq:1 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) -r3 -v user=foo require_user \
                   3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "failed.*Invalid" resout
    done
}

# -------------------------------------------------------------------------
# Tests for the "timeout" meta-data property.
# -------------------------------------------------------------------------

atf_test_case timeout
timeout_head()
{
    atf_set "descr" "Tests that 'timeout' works"
}
timeout_body()
{
    for h in $(get_helpers); do
        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) \
                   -v timeout=0 -v sleep=1 \
                   -r3 timeout 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "timeout, passed" resout

        atf_check -s eq:0 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) \
                   -v timeout=10 -v sleep=1 \
                   -r3 timeout 3>resout"
        atf_check -s eq:0 -o ignore -e empty grep "timeout, passed" resout

        atf_check -s eq:1 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) \
                   -v timeout=1 -v sleep=10 \
                   -r3 timeout 3>resout"
        atf_check -s eq:0 -o ignore -e empty \
                  grep "timeout, failed,.*timed out" resout

        atf_check -s eq:1 -o ignore -e ignore -x \
                  "${h} -s $(atf_get_srcdir) \
                   -v timeout=1 -v sleep=10 -v timeout2=10 -v sleep2=1 \
                   -r3 timeout timeout2 3>resout"
        atf_check -s eq:0 -o ignore -e empty -x \
                  'grep "timeout, failed,.*timed out" resout'
        atf_check -s eq:0 -o ignore -e empty grep "timeout2, passed" resout
    done
}

# -------------------------------------------------------------------------
# Main.
# -------------------------------------------------------------------------

atf_init_test_cases()
{
    atf_add_test_case ident
    atf_add_test_case require_arch
    atf_add_test_case require_config
    atf_add_test_case require_machine
    atf_add_test_case require_progs_func
    atf_add_test_case require_progs_header
    atf_add_test_case require_user_root
    atf_add_test_case require_user_unprivileged
    atf_add_test_case require_user_multiple
    atf_add_test_case require_user_bad
    atf_add_test_case timeout
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
