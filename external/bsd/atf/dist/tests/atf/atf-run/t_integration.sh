#
# Automated Testing Framework (atf)
#
# Copyright (c) 2007, 2008, 2009, 2010 The NetBSD Foundation, Inc.
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

create_helper_stdin()
{
    # TODO: This really, really, really must use real test programs.
    cat >${1} <<EOF
#! $(atf-config -t atf_shell)
while [ \${#} -gt 0 ]; do
    case \${1} in
        -l)
            echo 'Content-Type: application/X-atf-tp; version="1"'
            echo
EOF
    cnt=1
    while [ ${cnt} -le ${2} ]; do
        echo "echo 'ident: tc${cnt}'" >>${1}
        [ ${cnt} -lt ${2} ] && echo "echo" >>${1}
        cnt=$((${cnt} + 1))
    done
cat >>${1} <<EOF
            exit 0
            ;;
        -r*)
            resfile=\$(echo \${1} | cut -d r -f 2-)
            ;;
    esac
    testcase=\$(echo \${1} | cut -d : -f 1)
    shift
done
EOF
    cat >>${1}
}

atf_test_case config
config_head()
{
    atf_set "descr" "Tests that the config files are read in the correct" \
                    "order"
    atf_set "use.fs" "true"
}
config_body()
{
    create_helper config

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
    atf_check -s eq:0 -o save:stdout -e ignore -x \
        "ATF_CONFDIR=$(pwd)/etc HOME=$(pwd) atf-run helper"
    atf_check -s eq:0 -o ignore -e ignore grep '1st: sw common' stdout
    atf_check -s eq:0 -o ignore -e ignore grep '2nd: sw common' stdout
    atf_check -s eq:0 -o ignore -e ignore grep '3rd: sw common' stdout
    atf_check -s eq:0 -o ignore -e ignore grep '4th: sw common' stdout

    echo "Second: read system-wide <test-suite>.conf."
    cat >etc/atf.conf <<EOF
Content-Type: application/X-atf-config; version="1"

1st = "sw atf"
EOF
    atf_check -s eq:0 -o save:stdout -e ignore -x \
        "ATF_CONFDIR=$(pwd)/etc HOME=$(pwd) atf-run helper"
    atf_check -s eq:0 -o ignore -e ignore grep '1st: sw atf' stdout
    atf_check -s eq:0 -o ignore -e ignore grep '2nd: sw common' stdout
    atf_check -s eq:0 -o ignore -e ignore grep '3rd: sw common' stdout
    atf_check -s eq:0 -o ignore -e ignore grep '4th: sw common' stdout

    echo "Third: read user-specific common.conf."
    cat >.atf/common.conf <<EOF
Content-Type: application/X-atf-config; version="1"

2nd = "us common"
EOF
    atf_check -s eq:0 -o save:stdout -e ignore -x \
        "ATF_CONFDIR=$(pwd)/etc HOME=$(pwd) atf-run helper"
    atf_check -s eq:0 -o ignore -e ignore grep '1st: sw atf' stdout
    atf_check -s eq:0 -o ignore -e ignore grep '2nd: us common' stdout
    atf_check -s eq:0 -o ignore -e ignore grep '3rd: sw common' stdout
    atf_check -s eq:0 -o ignore -e ignore grep '4th: sw common' stdout

    echo "Fourth: read user-specific <test-suite>.conf."
    cat >.atf/atf.conf <<EOF
Content-Type: application/X-atf-config; version="1"

3rd = "us atf"
EOF
    atf_check -s eq:0 -o save:stdout -e ignore -x \
        "ATF_CONFDIR=$(pwd)/etc HOME=$(pwd) atf-run helper"
    atf_check -s eq:0 -o ignore -e ignore grep '1st: sw atf' stdout
    atf_check -s eq:0 -o ignore -e ignore grep '2nd: us common' stdout
    atf_check -s eq:0 -o ignore -e ignore grep '3rd: us atf' stdout
    atf_check -s eq:0 -o ignore -e ignore grep '4th: sw common' stdout
}

atf_test_case vflag
vflag_head()
{
    atf_set "descr" "Tests that the -v flag works and that it properly" \
                    "overrides the values in configuration files"
    atf_set "use.fs" "true"
}
vflag_body()
{
    create_helper testvar

    echo "Checking that 'testvar' is not defined."
    atf_check -s eq:1 -o ignore -e ignore -x \
        "ATF_CONFDIR=$(pwd)/etc atf-run helper"

    echo "Checking that defining 'testvar' trough '-v' works."
    atf_check -s eq:0 -o save:stdout -e ignore -x \
        "ATF_CONFDIR=$(pwd)/etc atf-run -v testvar='a value' helper"
    atf_check -s eq:0 -o ignore -e ignore grep 'testvar: a value' stdout

    echo "Checking that defining 'testvar' trough the configuration" \
         "file works."
    mkdir etc
    cat >etc/common.conf <<EOF
Content-Type: application/X-atf-config; version="1"

testvar = "value in conf file"
EOF
    atf_check -s eq:0 -o save:stdout -e ignore -x \
              "ATF_CONFDIR=$(pwd)/etc atf-run helper"
    atf_check -s eq:0 -o ignore -e ignore \
              grep 'testvar: value in conf file' stdout

    echo "Checking that defining 'testvar' trough -v overrides the" \
         "configuration file."
    atf_check -s eq:0 -o save:stdout -e ignore -x \
        "ATF_CONFDIR=$(pwd)/etc atf-run -v testvar='a value' helper"
    atf_check -s eq:0 -o ignore -e ignore grep 'testvar: a value' stdout
}

atf_test_case atffile
atffile_head()
{
    atf_set "descr" "Tests that the variables defined by the Atffile" \
                    "are recognized and that they take the lowest priority"
    atf_set "use.fs" "true"
}
atffile_body()
{
    create_helper testvar

    echo "Checking that 'testvar' is not defined."
    atf_check -s eq:1 -o ignore -e ignore -x \
              "ATF_CONFDIR=$(pwd)/etc atf-run helper"

    echo "Checking that defining 'testvar' trough the Atffile works."
    echo 'conf: testvar = "a value"' >>Atffile
    atf_check -s eq:0 -o save:stdout -e ignore -x \
              "ATF_CONFDIR=$(pwd)/etc atf-run helper"
    atf_check -s eq:0 -o ignore -e ignore \
              grep 'testvar: a value' stdout

    echo "Checking that defining 'testvar' trough the configuration" \
         "file overrides the one in the Atffile."
    mkdir etc
    cat >etc/common.conf <<EOF
Content-Type: application/X-atf-config; version="1"

testvar = "value in conf file"
EOF
    atf_check -s eq:0 -o save:stdout -e ignore -x \
              "ATF_CONFDIR=$(pwd)/etc atf-run helper"
    atf_check -s eq:0 -o ignore -e ignore \
              grep 'testvar: value in conf file' stdout
    rm -rf etc

    echo "Checking that defining 'testvar' trough -v overrides the" \
         "one in the Atffile."
    atf_check -s eq:0 -o save:stdout -e ignore -x \
        "ATF_CONFDIR=$(pwd)/etc atf-run -v testvar='new value' helper"
    atf_check -s eq:0 -o ignore -e ignore grep 'testvar: new value' stdout
}

atf_test_case atffile_recursive
atffile_recursive_head()
{
    atf_set "descr" "Tests that variables defined by an Atffile are not" \
                    "inherited by other Atffiles."
    atf_set "use.fs" "true"
}
atffile_recursive_body()
{
    create_helper testvar

    mkdir dir
    mv Atffile helper dir

    echo "Checking that 'testvar' is not inherited."
    create_atffile dir
    echo 'conf: testvar = "a value"' >> Atffile
    atf_check -s eq:1 -o ignore -e ignore -x "ATF_CONFDIR=$(pwd)/etc atf-run"

    echo "Checking that defining 'testvar' in the correct Atffile works."
    echo 'conf: testvar = "a value"' >>dir/Atffile
    atf_check -s eq:0 -o save:stdout -e ignore -x \
              "ATF_CONFDIR=$(pwd)/etc atf-run"
    atf_check -s eq:0 -o ignore -e ignore grep 'testvar: a value' stdout
}

atf_test_case fds
fds_head()
{
    atf_set "descr" "Tests that all streams are properly captured"
    atf_set "use.fs" "true"
}
fds_body()
{
    create_helper fds

    atf_check -s eq:0 -o save:stdout -e empty atf-run
    atf_check -s eq:0 -o ignore -e empty grep '^tc-so:msg1 to stdout$' stdout
    atf_check -s eq:0 -o ignore -e empty grep '^tc-so:msg2 to stdout$' stdout
    atf_check -s eq:0 -o ignore -e empty grep '^tc-se:msg1 to stderr$' stdout
    atf_check -s eq:0 -o ignore -e empty grep '^tc-se:msg2 to stderr$' stdout
}

atf_test_case missing_results
missing_results_head()
{
    atf_set "descr" "Ensures that atf-run correctly handles test cases that " \
                    "do not create the results file"
    atf_set "use.fs" "true"
}
missing_results_body()
{
    create_helper_stdin helper 1 <<EOF
test -f \${resfile} && echo "resfile found"
exit 0
EOF
    chmod +x helper

    create_atffile helper

    atf_check -s eq:1 -o save:stdout -e empty atf-run
    atf_check -s eq:0 -o ignore -e empty \
              grep '^tc-end: tc1, failed,.*failed to create' stdout
    atf_check -s eq:1 -o ignore -e empty grep 'resfile found' stdout
}

atf_test_case broken_results
broken_results_head()
{
    atf_set "descr" "Ensures that atf-run reports test programs that" \
                    "provide a bogus results output as broken programs"
    atf_set "use.fs" "true"
}
broken_results_body()
{
    # We produce two errors from the header to ensure that the parse
    # errors are printed on a single line on the output file.  Printing
    # them on separate lines would be incorrect.
    create_helper_stdin helper 1 <<EOF
echo 'foo' >\${resfile}
echo 'bar' >>\${resfile}
exit 0
EOF
    chmod +x helper

    create_atffile helper

    atf_check -s eq:1 -o save:stdout -e empty atf-run
    atf_check -s eq:0 -o ignore -e empty \
              grep '^tc-end: tc1, .*1:.*2:.*' stdout
}

atf_test_case broken_tp_list
broken_tp_list_head()
{
    atf_set "descr" "Ensures that atf-run reports test programs that" \
                    "provide a bogus test case list"
    atf_set "use.fs" "true"
}
broken_tp_list_body()
{
    cat >helper <<EOF
#! $(atf-config -t atf_shell)
while [ \${#} -gt 0 ]; do
    if [ \${1} = -l ]; then
        echo 'Content-Type: application/X-atf-tp; version="1"'
        echo
        echo 'foo: bar'
        exit 0
    else
        shift
    fi
done
exit 0
EOF
    chmod +x helper

    create_atffile helper

    atf_check -s eq:1 -o save:stdout -e empty atf-run
    atf_check -s eq:0 -o save:stdout -e empty grep '^tp-end: helper, ' stdout
    atf_check -s eq:0 -o ignore -e empty \
        grep 'Invalid format for test case list:.*First property.*ident' stdout
}

atf_test_case zero_tcs
zero_tcs_head()
{
    atf_set "descr" "Ensures that atf-run reports test programs without" \
                    "test cases as errors"
    atf_set "use.fs" "true"
}
zero_tcs_body()
{
    create_helper_stdin helper 0 <<EOF
echo 'Content-Type: application/X-atf-tp; version="1"'
echo
exit 1
EOF
    chmod +x helper

    create_atffile helper

    atf_check -s eq:1 -o save:stdout -e empty atf-run
    atf_check -s eq:0 -o ignore -e empty \
        grep '^tp-end: helper,.*Invalid format for test case list' stdout
}

atf_test_case exit_codes
exit_codes_head()
{
    atf_set "descr" "Ensures that atf-run reports bogus exit codes for" \
                    "programs correctly"
    atf_set "use.fs" "true"
}
exit_codes_body()
{
    create_helper_stdin helper 1 <<EOF
echo 'Content-Type: application/X-atf-tcr; version="1"' >\${resfile}
echo >>\${resfile}
echo "result: failed" >>\${resfile}
echo "reason: Yes, it failed" >>\${resfile}
exit 0
EOF
    chmod +x helper

    create_atffile helper

    atf_check -s eq:1 -o save:stdout -e empty atf-run
    atf_check -s eq:0 -o ignore -e empty \
        grep '^tc-end: tc1,.*exited successfully.*reported failure' stdout
}

atf_test_case signaled
signaled_head()
{
    atf_set "descr" "Ensures that atf-run reports test program's crashes" \
                    "correctly"
    atf_set "use.fs" "true"
}
signaled_body()
{
    create_helper_stdin helper 2 <<EOF
echo 'Content-Type: application/X-atf-tcr; version="1"' >\${resfile}
echo >>\${resfile}
echo "result: passed" >>\${resfile}
case \${testcase} in
    tc1) ;;
    tc2) echo "Killing myself!" ; kill -9 \$\$ ;;
esac
EOF
    chmod +x helper

    create_atffile helper

    atf_check -s eq:1 -o save:stdout -e empty atf-run
    atf_check -s eq:0 -o save:stdout -e empty grep '^tc-end: tc2, ' stdout
    atf_check -s eq:0 -o ignore -e empty grep 'received signal 9' stdout
}

atf_test_case no_reason
no_reason_head()
{
    atf_set "descr" "Ensures that atf-run reports bogus test programs" \
                    "that do not provide a reason for failed or skipped" \
                    "test cases"
    atf_set "use.fs" "true"
}
no_reason_body()
{
    for r in failed skipped; do
        create_helper_stdin helper 1 <<EOF
echo 'Content-Type: application/X-atf-tcr; version="1"' >\${resfile}
echo '' >>\${resfile}
echo 'result: ${r}' >>\${resfile}
false
EOF
        chmod +x helper

        create_atffile helper

        atf_check -s eq:1 -o save:stdout -e empty atf-run
        atf_check -s eq:0 -o ignore -e empty \
                  grep '^tc-end: tc1,.*No reason specified' stdout
    done
}

atf_test_case hooks
hooks_head()
{
    atf_set "descr" "Checks that the default hooks work and that they" \
                    "can be overriden by the user"
    atf_set "use.fs" "true"
}
hooks_body()
{
    cp $(atf_get_srcdir)/h_pass helper
    create_atffile helper

    mkdir atf
    mkdir .atf

    echo "Checking default hooks"
    atf_check -s eq:0 -o save:stdout -e empty -x \
              "ATF_CONFDIR=$(pwd)/atf atf-run"
    atf_check -s eq:0 -o ignore -e empty grep '^info: time.start, ' stdout
    atf_check -s eq:0 -o ignore -e empty grep '^info: time.end, ' stdout

    echo "Checking the system-wide info_start hook"
    cat >atf/atf-run.hooks <<EOF
info_start_hook()
{
    atf_tps_writer_info "test" "sw value"
}
EOF
    atf_check -s eq:0 -o save:stdout -e empty -x \
              "ATF_CONFDIR=$(pwd)/atf atf-run"
    atf_check -s eq:0 -o ignore -e empty grep '^info: test, sw value' stdout
    atf_check -s eq:1 -o empty -e empty grep '^info: time.start, ' stdout
    atf_check -s eq:0 -o ignore -e empty grep '^info: time.end, ' stdout

    echo "Checking the user-specific info_start hook"
    cat >.atf/atf-run.hooks <<EOF
info_start_hook()
{
    atf_tps_writer_info "test" "user value"
}
EOF
    atf_check -s eq:0 -o save:stdout -e empty -x \
              "ATF_CONFDIR=$(pwd)/atf atf-run"
    atf_check -s eq:0 -o ignore -e empty \
              grep '^info: test, user value' stdout
    atf_check -s eq:1 -o empty -e empty grep '^info: time.start, ' stdout
    atf_check -s eq:0 -o ignore -e empty grep '^info: time.end, ' stdout

    rm atf/atf-run.hooks
    rm .atf/atf-run.hooks

    echo "Checking the system-wide info_end hook"
    cat >atf/atf-run.hooks <<EOF
info_end_hook()
{
    atf_tps_writer_info "test" "sw value"
}
EOF
    atf_check -s eq:0 -o save:stdout -e empty -x \
              "ATF_CONFDIR=$(pwd)/atf atf-run"
    atf_check -s eq:0 -o ignore -e empty grep '^info: time.start, ' stdout
    atf_check -s eq:1 -o empty -e empty grep '^info: time.end, ' stdout
    atf_check -s eq:0 -o ignore -e empty grep '^info: test, sw value' stdout

    echo "Checking the user-specific info_end hook"
    cat >.atf/atf-run.hooks <<EOF
info_end_hook()
{
    atf_tps_writer_info "test" "user value"
}
EOF
    atf_check -s eq:0 -o save:stdout -e empty -x \
              "ATF_CONFDIR=$(pwd)/atf atf-run"
    atf_check -s eq:0 -o ignore -e empty grep '^info: time.start, ' stdout
    atf_check -s eq:1 -o empty -e empty grep '^info: time.end, ' stdout
    atf_check -s eq:0 -o ignore -e empty \
              grep '^info: test, user value' stdout
}

atf_test_case isolation_env
isolation_env_head()
{
    atf_set "descr" "Tests that atf-run sets a set of environment variables" \
                    "to known sane values"
    atf_set "use.fs" "true"
}
isolation_env_body()
{
    undef_vars="LANG LC_ALL LC_COLLATE LC_CTYPE LC_MESSAGES LC_MONETARY \
                LC_NUMERIC LC_TIME TZ"
    def_vars="HOME"

    mangleenv="env"
    for v in ${undef_vars} ${def_vars}; do
        mangleenv="${mangleenv} ${v}=bogus-value"
    done

    create_helper env_list
    create_atffile helper

    # We must ignore stderr in this call (instead of specifying -e empty)
    # because, when atf-run invokes the shell to run the hooks, we may get
    # error messages about an invalid locale.  This happens, at least, when
    # the shell is bash 4.x.
    atf_check -s eq:0 -o save:stdout -e ignore ${mangleenv} atf-run helper

    for v in ${undef_vars}; do
        atf_check -s eq:1 -o empty -e empty grep "^tc-so:${v}=" stdout
    done

    for v in ${def_vars}; do
        atf_check -s eq:0 -o ignore -e empty grep "^tc-so:${v}=" stdout
    done
}

atf_test_case isolation_home
isolation_home_head()
{
    atf_set "descr" "Tests that atf-run sets HOME to a sane and valid value"
    atf_set "use.fs" "true"
}
isolation_home_body()
{
    create_helper env_home
    create_atffile helper
    atf_check -s eq:0 -o ignore -e ignore env HOME=foo atf-run helper
}

atf_test_case isolation_umask
isolation_umask_head()
{
    atf_set "descr" "Tests that atf-run sets the umask to a known value"
    atf_set "use.fs" "true"
}
isolation_umask_body()
{
    create_helper umask
    create_atffile helper

    atf_check -s eq:0 -o save:stdout -e ignore -x "umask 0000 && atf-run helper"
    atf_check -s eq:0 -o ignore -e empty grep 'umask: 0022' stdout
}

atf_test_case cleanup_pass
cleanup_pass_head()
{
    atf_set "descr" "Tests that atf-run calls the cleanup routine of the test" \
        "case when the test case result is passed"
    atf_set "use.fs" "true"
}
cleanup_pass_body()
{
    create_helper cleanup_states
    create_atffile helper

    atf_check -s eq:0 -o save:stdout -e ignore atf-run -v state=pass \
        -v statedir=$(pwd) helper
    atf_check -s eq:0 -o ignore -e empty grep 'cleanup_states, passed' stdout
    test -f to-stay || atf_fail "Test case body did not run correctly"
    test -f to-delete && atf_fail "Test case cleanup did not run correctly"
}

atf_test_case cleanup_fail
cleanup_fail_head()
{
    atf_set "descr" "Tests that atf-run calls the cleanup routine of the test" \
        "case when the test case result is failed"
    atf_set "use.fs" "true"
}
cleanup_fail_body()
{
    create_helper cleanup_states
    create_atffile helper

    atf_check -s eq:1 -o save:stdout -e ignore atf-run -v state=fail \
        -v statedir=$(pwd) helper
    atf_check -s eq:0 -o ignore -e empty grep 'cleanup_states, failed' stdout
    test -f to-stay || atf_fail "Test case body did not run correctly"
    test -f to-delete && atf_fail "Test case cleanup did not run correctly"
}

atf_test_case cleanup_skip
cleanup_skip_head()
{
    atf_set "descr" "Tests that atf-run calls the cleanup routine of the test" \
        "case when the test case result is skipped"
    atf_set "use.fs" "true"
}
cleanup_skip_body()
{
    create_helper cleanup_states
    create_atffile helper

    atf_check -s eq:0 -o save:stdout -e ignore atf-run -v state=skip \
        -v statedir=$(pwd) helper
    atf_check -s eq:0 -o ignore -e empty grep 'cleanup_states, skipped' stdout
    test -f to-stay || atf_fail "Test case body did not run correctly"
    test -f to-delete && atf_fail "Test case cleanup did not run correctly"
}

atf_test_case cleanup_curdir
cleanup_curdir_head()
{
    atf_set "descr" "Tests that atf-run calls the cleanup routine in the same" \
        "work directory as the body so that they can share data"
    atf_set "use.fs" "true"
}
cleanup_curdir_body()
{
    create_helper cleanup_curdir
    create_atffile helper

    atf_check -s eq:0 -o save:stdout -e ignore atf-run helper
    atf_check -s eq:0 -o ignore -e empty grep 'cleanup_curdir, passed' stdout
    atf_check -s eq:0 -o ignore -e empty grep 'Old value: 1234' stdout
}

atf_test_case cleanup_signal
cleanup_signal_head()
{
    atf_set "descr" "Tests that atf-run calls the cleanup routine if it gets" \
        "a termination signal while running the body"
}
cleanup_signal_body()
{
    : # TODO: Write this.
}

atf_test_case require_arch
require_arch_head()
{
    atf_set "descr" "Tests that atf-run validates the require.arch property"
    atf_set "use.fs" "true"
}
require_arch_body()
{
    create_helper require_arch
    create_atffile helper

    echo "Checking for the real architecture"
    arch=$(atf-config -t atf_arch)
    atf_check -s eq:0 -o save:so -e ignore atf-run -v arch="${arch}" helper
    atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so
    atf_check -s eq:0 -o save:so -e ignore atf-run -v arch="foo ${arch}" helper
    atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so
    atf_check -s eq:0 -o save:so -e ignore atf-run -v arch="${arch} foo" helper
    atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so

    echo "Checking for a fictitious architecture"
    arch=fictitious
    export ATF_ARCH=fictitious
    atf_check -s eq:0 -o save:so -e ignore atf-run -v arch="${arch}" helper
    atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so
    atf_check -s eq:0 -o save:so -e ignore atf-run -v arch="foo ${arch}" helper
    atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so
    atf_check -s eq:0 -o save:so -e ignore atf-run -v arch="${arch} foo" helper
    atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so

    echo "Triggering some failures"
    atf_check -s eq:0 -o save:so -e ignore atf-run -v arch="foo" helper
    atf_check -s eq:0 -o ignore -e empty grep \
        "${TESTCASE}, skipped, .*foo.*architecture" so
    atf_check -s eq:0 -o save:so -e ignore atf-run -v arch="foo bar" helper
    atf_check -s eq:0 -o ignore -e empty grep \
        "${TESTCASE}, skipped, .*foo bar.*architectures" so
    atf_check -s eq:0 -o save:so -e ignore atf-run -v arch="${arch}xxx" helper
    atf_check -s eq:0 -o ignore -e empty grep \
        "${TESTCASE}, skipped, .*fictitiousxxx.*architecture" so
}

atf_test_case require_config
require_config_head()
{
    atf_set "descr" "Tests that atf-run validates the require.config property"
    atf_set "use.fs" "true"
}
require_config_body()
{
    create_helper require_config
    create_atffile helper

    atf_check -s eq:0 -o save:so -e ignore atf-run helper
    atf_check -s eq:0 -o ignore -e empty grep \
        "${TESTCASE}, skipped, .*var1.*not defined" so

    atf_check -s eq:0 -o save:so -e ignore atf-run -v var1=foo helper
    atf_check -s eq:0 -o ignore -e empty grep \
        "${TESTCASE}, skipped, .*var2.*not defined" so

    atf_check -s eq:0 -o save:so -e ignore atf-run -v var1=a -v var2=' ' helper
    atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so
}

atf_test_case require_machine
require_machine_head()
{
    atf_set "descr" "Tests that atf-run validates the require.machine property"
    atf_set "use.fs" "true"
}
require_machine_body()
{
    create_helper require_machine
    create_atffile helper

    echo "Checking for the real machine type"
    machine=$(atf-config -t atf_machine)
    atf_check -s eq:0 -o save:so -e ignore atf-run -v machine="${machine}" \
        helper
    atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so
    atf_check -s eq:0 -o save:so -e ignore atf-run -v machine="foo ${machine}" \
        helper
    atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so
    atf_check -s eq:0 -o save:so -e ignore atf-run -v machine="${machine} foo" \
        helper
    atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so

    echo "Checking for a fictitious machine type"
    machine=fictitious
    export ATF_MACHINE=fictitious
    atf_check -s eq:0 -o save:so -e ignore atf-run -v machine="${machine}" \
        helper
    atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so
    atf_check -s eq:0 -o save:so -e ignore atf-run -v machine="foo ${machine}" \
        helper
    atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so
    atf_check -s eq:0 -o save:so -e ignore atf-run -v machine="${machine} foo" \
        helper
    atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so

    echo "Triggering some failures"
    atf_check -s eq:0 -o save:so -e ignore atf-run -v machine="foo" helper
    atf_check -s eq:0 -o ignore -e empty grep \
        "${TESTCASE}, skipped, .*foo.*machine type" so
    atf_check -s eq:0 -o save:so -e ignore atf-run -v machine="foo bar" helper
    atf_check -s eq:0 -o ignore -e empty grep \
        "${TESTCASE}, skipped, .*foo bar.*machine types" so
    atf_check -s eq:0 -o save:so -e ignore atf-run -v machine="${machine}xxx" \
        helper
    atf_check -s eq:0 -o ignore -e empty grep \
        "${TESTCASE}, skipped, .*fictitiousxxx.*machine type" so
}

atf_test_case require_progs
require_progs_head()
{
    atf_set "descr" "Tests that atf-run validates the require.progs property"
    atf_set "use.fs" "true"
}
require_progs_body()
{
    create_helper require_progs
    create_atffile helper

    echo "Checking absolute paths"
    atf_check -s eq:0 -o save:so -e ignore atf-run -v progs='/bin/cp' helper
    atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so
    atf_check -s eq:0 -o save:so -e ignore atf-run \
        -v progs='/bin/__non-existent__' helper
    atf_check -s eq:0 -o ignore -e empty grep \
        "${TESTCASE}, skipped, .*/bin/__non-existent__.*PATH" so

    echo "Checking that relative paths are not allowed"
    atf_check -s eq:1 -o save:so -e ignore atf-run -v progs='bin/cp' helper
    atf_check -s eq:0 -o ignore -e empty grep \
        "${TESTCASE}, failed, Relative paths.*not allowed.*bin/cp" so

    echo "Check plain file names, searching them in the PATH."
    atf_check -s eq:0 -o save:so -e ignore atf-run -v progs='cp' helper
    atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so
    atf_check -s eq:0 -o save:so -e ignore atf-run \
        -v progs='__non-existent__' helper
    atf_check -s eq:0 -o ignore -e empty grep \
        "${TESTCASE}, skipped, .*__non-existent__.*PATH" so
}

atf_test_case require_user_root
require_user_root_head()
{
    atf_set "descr" "Tests that atf-run validates the require.user property" \
        "when it is set to 'root'"
    atf_set "use.fs" "true"
}
require_user_root_body()
{
    create_helper require_user
    create_atffile helper

    atf_check -s eq:0 -o save:so -e ignore atf-run -v user=root helper
    if [ $(id -u) -eq 0 ]; then
        atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so
    else
        atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, skipped" so
    fi
}

atf_test_case require_user_unprivileged
require_user_unprivileged_head()
{
    atf_set "descr" "Tests that atf-run validates the require.user property" \
        "when it is set to 'root'"
    atf_set "use.fs" "true"
}
require_user_unprivileged_body()
{
    create_helper require_user
    create_atffile helper

    atf_check -s eq:0 -o save:so -e ignore atf-run -v user=unprivileged helper
    if [ $(id -u) -eq 0 ]; then
        atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, skipped" so
    else
        atf_check -s eq:0 -o ignore -e empty grep "${TESTCASE}, passed" so
    fi
}

atf_test_case require_user_bad
require_user_bad_head()
{
    atf_set "descr" "Tests that atf-run validates the require.user property" \
        "when it is set to 'root'"
    atf_set "use.fs" "true"
}
require_user_bad_body()
{
    create_helper require_user
    create_atffile helper

    atf_check -s eq:1 -o save:so -e ignore atf-run -v user=foobar helper
    atf_check -s eq:0 -o ignore -e empty grep \
        "${TESTCASE}, failed, Invalid value.*foobar" so
}

atf_test_case timeout
timeout_head()
{
    atf_set "descr" "Tests that atf-run kills a test case that times out"
    atf_set "use.fs" "true"
}
timeout_body()
{
    create_helper timeout
    create_atffile helper

    atf_check -s eq:1 -o save:stdout -e ignore atf-run -v statedir=$(pwd) helper
    test -f finished && atf_fail "Test case was not killed after time out"
    atf_check -s eq:0 -o ignore -e empty grep \
        "${TESTCASE}, failed, .*timed out after 1 second" stdout
}

atf_test_case use_fs
use_fs_head()
{
    atf_set "descr" "Tests that atf-run correctly handles the use.fs property"
    atf_set "use.fs" "true"
}
use_fs_body()
{
    create_helper use_fs
    create_atffile helper

    atf_check -s eq:0 -o ignore -e ignore atf-run \
        -v allowed=false -v access=false helper

    atf_check -s eq:1 -o ignore -e ignore atf-run \
        -v allowed=false -v access=true helper

    atf_check -s eq:0 -o ignore -e ignore atf-run \
        -v allowed=true -v access=false helper

    atf_check -s eq:0 -o ignore -e ignore atf-run \
        -v allowed=true -v access=true helper
}

atf_init_test_cases()
{
    atf_add_test_case config
    atf_add_test_case vflag
    atf_add_test_case atffile
    atf_add_test_case atffile_recursive
    atf_add_test_case fds
    atf_add_test_case missing_results
    atf_add_test_case broken_results
    atf_add_test_case broken_tp_list
    atf_add_test_case zero_tcs
    atf_add_test_case exit_codes
    atf_add_test_case signaled
    atf_add_test_case no_reason
    atf_add_test_case hooks
    atf_add_test_case isolation_env
    atf_add_test_case isolation_home
    atf_add_test_case isolation_umask
    atf_add_test_case cleanup_pass
    atf_add_test_case cleanup_fail
    atf_add_test_case cleanup_skip
    atf_add_test_case cleanup_curdir
    atf_add_test_case cleanup_signal
    atf_add_test_case require_arch
    atf_add_test_case require_config
    atf_add_test_case require_machine
    atf_add_test_case require_progs
    atf_add_test_case require_user_root
    atf_add_test_case require_user_unprivileged
    atf_add_test_case require_user_bad
    atf_add_test_case timeout
    atf_add_test_case use_fs
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
