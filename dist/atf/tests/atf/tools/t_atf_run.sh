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

create_atffile()
{
    cat >Atffile <<EOF
Content-Type: application/X-atf-atffile; version="1"

prop: test-suite = atf

EOF
    for f in "${@}"; do
        echo "tp: ${f}" >>Atffile
    done
}

create_helper()
{
    cp $(atf_get_srcdir)/h_misc helper
    create_atffile helper
    TESTCASE=${1}; export TESTCASE
}

atf_test_case config
config_head()
{
    atf_set "descr" "Tests that the config files are read in the correct" \
                    "order"
}
config_body()
{
    create_helper atf_run_config

    mkdir etc
    mkdir .atf

    echo "First: read system-wide common.conf."
    cat >etc/common.conf <<EOF
Content-Type: application/X-atf-config; version="1"

1st = "sw common"
2nd = "sw common"
3rd = "sw common"
4th = "sw common"
EOF
    atf_check "ATF_CONFDIR=$(pwd)/etc HOME=$(pwd) atf-run helper" \
              0 stdout ignore
    atf_check "grep '1st: sw common' stdout" 0 ignore ignore
    atf_check "grep '2nd: sw common' stdout" 0 ignore ignore
    atf_check "grep '3rd: sw common' stdout" 0 ignore ignore
    atf_check "grep '4th: sw common' stdout" 0 ignore ignore

    echo "Second: read system-wide <test-suite>.conf."
    cat >etc/atf.conf <<EOF
Content-Type: application/X-atf-config; version="1"

1st = "sw atf"
EOF
    atf_check "ATF_CONFDIR=$(pwd)/etc HOME=$(pwd) atf-run helper" \
              0 stdout ignore
    atf_check "grep '1st: sw atf' stdout" 0 ignore ignore
    atf_check "grep '2nd: sw common' stdout" 0 ignore ignore
    atf_check "grep '3rd: sw common' stdout" 0 ignore ignore
    atf_check "grep '4th: sw common' stdout" 0 ignore ignore

    echo "Third: read user-specific common.conf."
    cat >.atf/common.conf <<EOF
Content-Type: application/X-atf-config; version="1"

2nd = "us common"
EOF
    atf_check "ATF_CONFDIR=$(pwd)/etc HOME=$(pwd) atf-run helper" \
              0 stdout ignore
    atf_check "grep '1st: sw atf' stdout" 0 ignore ignore
    atf_check "grep '2nd: us common' stdout" 0 ignore ignore
    atf_check "grep '3rd: sw common' stdout" 0 ignore ignore
    atf_check "grep '4th: sw common' stdout" 0 ignore ignore

    echo "Fourth: read user-specific <test-suite>.conf."
    cat >.atf/atf.conf <<EOF
Content-Type: application/X-atf-config; version="1"

3rd = "us atf"
EOF
    atf_check "ATF_CONFDIR=$(pwd)/etc HOME=$(pwd) atf-run helper" \
              0 stdout ignore
    atf_check "grep '1st: sw atf' stdout" 0 ignore ignore
    atf_check "grep '2nd: us common' stdout" 0 ignore ignore
    atf_check "grep '3rd: us atf' stdout" 0 ignore ignore
    atf_check "grep '4th: sw common' stdout" 0 ignore ignore
}

atf_test_case vflag
vflag_head()
{
    atf_set "descr" "Tests that the -v flag works and that it properly" \
                    "overrides the values in configuration files"
}
vflag_body()
{
    create_helper atf_run_testvar

    echo "Checking that 'testvar' is not defined."
    atf_check "ATF_CONFDIR=$(pwd)/etc atf-run helper" 1 ignore ignore

    echo "Checking that defining 'testvar' trough '-v' works."
    atf_check "ATF_CONFDIR=$(pwd)/etc atf-run -v testvar='a value' helper" \
              0 stdout ignore
    atf_check "grep 'testvar: a value' stdout" 0 ignore ignore

    echo "Checking that defining 'testvar' trough the configuration" \
         "file works."
    mkdir etc
    cat >etc/common.conf <<EOF
Content-Type: application/X-atf-config; version="1"

testvar = "value in conf file"
EOF
    atf_check "ATF_CONFDIR=$(pwd)/etc atf-run helper" 0 stdout ignore
    atf_check "grep 'testvar: value in conf file' stdout" 0 ignore ignore

    echo "Checking that defining 'testvar' trough -v overrides the" \
         "configuration file."
    atf_check "ATF_CONFDIR=$(pwd)/etc atf-run -v testvar='a value' helper" \
              0 stdout ignore
    atf_check "grep 'testvar: a value' stdout" 0 ignore ignore
}

atf_test_case atffile
atffile_head()
{
    atf_set "descr" "Tests that the variables defined by the Atffile" \
                    "are recognized and that they take the lowest priority"
}
atffile_body()
{
    create_helper atf_run_testvar

    echo "Checking that 'testvar' is not defined."
    atf_check "ATF_CONFDIR=$(pwd)/etc atf-run helper" 1 ignore ignore

    echo "Checking that defining 'testvar' trough the Atffile works."
    echo 'conf: testvar = "a value"' >>Atffile
    atf_check "ATF_CONFDIR=$(pwd)/etc atf-run helper" 0 stdout ignore
    atf_check "grep 'testvar: a value' stdout" 0 ignore ignore

    echo "Checking that defining 'testvar' trough the configuration" \
         "file overrides the one in the Atffile."
    mkdir etc
    cat >etc/common.conf <<EOF
Content-Type: application/X-atf-config; version="1"

testvar = "value in conf file"
EOF
    atf_check "ATF_CONFDIR=$(pwd)/etc atf-run helper" 0 stdout ignore
    atf_check "grep 'testvar: value in conf file' stdout" 0 ignore ignore
    rm -rf etc

    echo "Checking that defining 'testvar' trough -v overrides the" \
         "one in the Atffile."
    atf_check "ATF_CONFDIR=$(pwd)/etc atf-run -v testvar='new value' helper" \
              0 stdout ignore
    atf_check "grep 'testvar: new value' stdout" 0 ignore ignore
}

atf_test_case atffile_recursive
atffile_recursive_head()
{
    atf_set "descr" "Tests that variables defined by an Atffile are not" \
                    "inherited by other Atffiles."
}
atffile_recursive_body()
{
    create_helper atf_run_testvar

    mkdir dir
    mv Atffile helper dir

    echo "Checking that 'testvar' is not inherited."
    create_atffile dir
    echo 'conf: testvar = "a value"' >> Atffile
    atf_check "ATF_CONFDIR=$(pwd)/etc atf-run" 1 ignore ignore

    echo "Checking that defining 'testvar' in the correct Atffile works."
    echo 'conf: testvar = "a value"' >>dir/Atffile
    atf_check "ATF_CONFDIR=$(pwd)/etc atf-run" 0 stdout ignore
    atf_check "grep 'testvar: a value' stdout" 0 ignore ignore
}

atf_test_case fds
fds_head()
{
    atf_set "descr" "Tests that all streams are properly captured"
}
fds_body()
{
    create_helper atf_run_fds

    atf_check "atf-run" 0 stdout null
    atf_check "grep '^tc-so:msg1 to stdout$' stdout" 0 ignore null
    atf_check "grep '^tc-so:msg2 to stdout$' stdout" 0 ignore null
    atf_check "grep '^tc-se:msg1 to stderr$' stdout" 0 ignore null
    atf_check "grep '^tc-se:msg2 to stderr$' stdout" 0 ignore null
}

atf_test_case broken_tp_hdr
broken_tp_hdr_head()
{
    atf_set "descr" "Ensures that atf-run reports test programs that" \
                    "provide a bogus header as broken programs"
}
broken_tp_hdr_body()
{
    # We produce two errors from the header to ensure that the parse
    # errors are printed on a single line on the output file.  Printing
    # them on separate lines would be incorrect.
    cat >helper <<EOF
#! $(atf-config -t atf_shell)
echo 'foo' 1>&9
echo 'bar' 1>&9
exit 0
EOF
    chmod +x helper

    create_atffile helper

    atf_check "atf-run" 1 stdout null
    atf_check "grep '^tp-end: helper, .*Line 1.*Line 2' stdout" 0 ignore null
}

atf_test_case zero_tcs
zero_tcs_head()
{
    atf_set "descr" "Ensures that atf-run reports test programs without" \
                    "test cases as errors"
}
zero_tcs_body()
{
    cat >helper <<EOF
#! $(atf-config -t atf_shell)
echo 'Content-Type: application/X-atf-tcs; version="1"' 1>&9
echo 1>&9
echo "tcs-count: 0" 1>&9
exit 0
EOF
    chmod +x helper

    create_atffile helper

    atf_check "atf-run" 1 stdout null
    atf_check "grep '^tp-end: helper, ' stdout" 0 stdout null
    atf_check "grep '0 test cases' stdout" 0 ignore null
}

atf_test_case exit_codes
exit_codes_head()
{
    atf_set "descr" "Ensures that atf-run reports bogus exit codes for" \
                    "programs correctly"
}
exit_codes_body()
{
    cat >helper <<EOF
#! $(atf-config -t atf_shell)
echo 'Content-Type: application/X-atf-tcs; version="1"' 1>&9
echo 1>&9
echo "tcs-count: 1" 1>&9
echo "tc-start: foo" 1>&9
echo "tc-end: foo, failed, Yes, it failed" 1>&9
true
EOF
    chmod +x helper

    create_atffile helper

    atf_check "atf-run" 1 stdout null
    atf_check "grep '^tp-end: helper, ' stdout" 0 stdout null
    atf_check "grep 'success.*test cases failed' stdout" 0 ignore null
}

atf_test_case signaled
signaled_head()
{
    atf_set "descr" "Ensures that atf-run reports test program's crashes" \
                    "correctly"
}
signaled_body()
{
    cat >helper <<EOF
#! $(atf-config -t atf_shell)
echo 'Content-Type: application/X-atf-tcs; version="1"' 1>&9
echo 1>&9
echo "tcs-count: 1" 1>&9
echo "tc-start: foo" 1>&9
echo "tc-end: foo, failed, Will fail" 1>&9
kill -9 \$\$
EOF
    chmod +x helper

    create_atffile helper

    atf_check "atf-run" 1 stdout null
    atf_check "grep '^tp-end: helper, ' stdout" 0 stdout null
    atf_check "grep 'received signal 9' stdout" 0 ignore null
}

atf_test_case no_reason
no_reason_head()
{
    atf_set "descr" "Ensures that atf-run reports bogus test programs" \
                    "that do not provide a reason for failed or skipped" \
                    "test cases"
}
no_reason_body()
{
    for r in failed skipped; do
        cat >helper <<EOF
#! $(atf-config -t atf_shell)
echo 'Content-Type: application/X-atf-tcs; version="1"' 1>&9
echo 1>&9
echo "tcs-count: 1" 1>&9
echo "tc-start: foo" 1>&9
echo "tc-end: foo, ${r}" 1>&9
false
EOF
        chmod +x helper

        create_atffile helper

        atf_check "atf-run" 1 stdout null
        atf_check "grep '^tp-end: helper, ' stdout" 0 stdout null
        atf_check "grep 'Unexpected.*NEWLINE' stdout" 0 ignore null
    done
}

atf_test_case hooks
hooks_head()
{
    atf_set "descr" "Checks that the default hooks work and that they" \
                    "can be overriden by the user"
}
hooks_body()
{
    cp $(atf_get_srcdir)/h_pass helper
    create_atffile helper

    mkdir atf
    mkdir .atf

    echo "Checking default hooks"
    atf_check "ATF_CONFDIR=$(pwd)/atf atf-run" 0 stdout null
    atf_check "grep '^info: time.start, ' stdout" 0 ignore null
    atf_check "grep '^info: time.end, ' stdout" 0 ignore null

    echo "Checking the system-wide info_start hook"
    cat >atf/atf-run.hooks <<EOF
info_start_hook()
{
    atf_tps_writer_info "test" "sw value"
}
EOF
    atf_check "ATF_CONFDIR=$(pwd)/atf atf-run" 0 stdout null
    atf_check "grep '^info: test, sw value' stdout" 0 ignore null
    atf_check "grep '^info: time.start, ' stdout" 1 null null
    atf_check "grep '^info: time.end, ' stdout" 0 ignore null

    echo "Checking the user-specific info_start hook"
    cat >.atf/atf-run.hooks <<EOF
info_start_hook()
{
    atf_tps_writer_info "test" "user value"
}
EOF
    atf_check "ATF_CONFDIR=$(pwd)/atf atf-run" 0 stdout null
    atf_check "grep '^info: test, user value' stdout" 0 ignore null
    atf_check "grep '^info: time.start, ' stdout" 1 null null
    atf_check "grep '^info: time.end, ' stdout" 0 ignore null

    rm atf/atf-run.hooks
    rm .atf/atf-run.hooks

    echo "Checking the system-wide info_end hook"
    cat >atf/atf-run.hooks <<EOF
info_end_hook()
{
    atf_tps_writer_info "test" "sw value"
}
EOF
    atf_check "ATF_CONFDIR=$(pwd)/atf atf-run" 0 stdout null
    atf_check "grep '^info: time.start, ' stdout" 0 ignore null
    atf_check "grep '^info: time.end, ' stdout" 1 null null
    atf_check "grep '^info: test, sw value' stdout" 0 ignore null

    echo "Checking the user-specific info_end hook"
    cat >.atf/atf-run.hooks <<EOF
info_end_hook()
{
    atf_tps_writer_info "test" "user value"
}
EOF
    atf_check "ATF_CONFDIR=$(pwd)/atf atf-run" 0 stdout null
    atf_check "grep '^info: time.start, ' stdout" 0 ignore null
    atf_check "grep '^info: time.end, ' stdout" 1 null null
    atf_check "grep '^info: test, user value' stdout" 0 ignore null
}

atf_init_test_cases()
{
    atf_add_test_case config
    atf_add_test_case vflag
    atf_add_test_case atffile
    atf_add_test_case atffile_recursive
    atf_add_test_case fds
    atf_add_test_case broken_tp_hdr
    atf_add_test_case zero_tcs
    atf_add_test_case exit_codes
    atf_add_test_case signaled
    atf_add_test_case no_reason
    atf_add_test_case hooks
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
