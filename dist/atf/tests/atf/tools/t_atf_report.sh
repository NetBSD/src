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

create_helpers()
{
    mkdir dir1
    cp $(atf_get_srcdir)/h_pass dir1/tp1
    cp $(atf_get_srcdir)/h_fail dir1/tp2
    cp $(atf_get_srcdir)/h_pass tp3
    cp $(atf_get_srcdir)/h_fail tp4

    cat >tp5 <<EOF
#! /bin/sh
echo foo
EOF
    chmod +x tp5

    cat >Atffile <<EOF
Content-Type: application/X-atf-atffile; version="1"

prop: test-suite = atf

tp: dir1
tp: tp3
tp: tp4
tp: tp5
EOF

    cat >dir1/Atffile <<EOF
Content-Type: application/X-atf-atffile; version="1"

prop: test-suite = atf

tp: tp1
tp: tp2
EOF
}

run_helpers()
{
    mkdir etc
    cat >etc/atf-run.hooks <<EOF
#! /bin/sh

info_start_hook()
{
    atf_tps_writer_info "startinfo" "A value"
}

info_end_hook()
{
    atf_tps_writer_info "endinfo" "Another value"
}
EOF
    echo "Using atf-run to run helpers"
    ATF_CONFDIR=$(pwd)/etc atf-run >tps.out 2>/dev/null
    rm -rf etc
}

atf_test_case default
default_head()
{
    atf_set "descr" "Checks that the default output uses the ticker" \
                    "format"
}
default_body()
{
    create_helpers
    run_helpers

    # Check that the default output uses the ticker format.
    atf_check 'atf-report <tps.out' 0 stdout null
    atf_check 'grep "test cases" stdout' 0 ignore null
    atf_check 'grep "Failed test cases" stdout' 0 ignore null
    atf_check 'grep "Summary for" stdout' 0 ignore null
}

atf_test_case oflag
oflag_head()
{
    atf_set "descr" "Checks that the -o flag works"
}
oflag_body()
{
    create_helpers
    run_helpers

    # Get the default output.
    atf_check 'atf-report <tps.out' 0 stdout null
    mv stdout defout

    # Check that changing the stdout output works.
    atf_check 'atf-report -o csv:- <tps.out' 0 stdout null
    atf_check 'cmp -s defout stdout' 1 null null
    cp stdout expcsv

    # Check that sending the output to a file does not write to stdout.
    atf_check 'atf-report -o csv:fmt.out <tps.out' 0 null null
    atf_check 'cmp -s expcsv fmt.out' 0 null null
    rm -f fmt.out

    # Check that defining two outputs using the same format works.
    atf_check 'atf-report -o csv:fmt.out -o csv:fmt2.out <tps.out' \
              0 null null
    atf_check 'cmp -s expcsv fmt.out' 0 null null
    atf_check 'cmp -s fmt.out fmt2.out' 0 null null
    rm -f fmt.out fmt2.out

    # Check that defining two outputs using different formats works.
    atf_check 'atf-report -o csv:fmt.out -o ticker:fmt2.out <tps.out' \
              0 null null
    atf_check 'cmp -s expcsv fmt.out' 0 null null
    atf_check 'cmp -s fmt.out fmt2.out' 1 null null
    atf_check 'grep "test cases" fmt2.out' 0 ignore null
    atf_check 'grep "Failed test cases" fmt2.out' 0 ignore null
    atf_check 'grep "Summary for" fmt2.out' 0 ignore null
    rm -f fmt.out fmt2.out

    # Check that defining two outputs over the same file does not work.
    atf_check 'atf-report -o csv:fmt.out -o ticker:fmt.out <tps.out' \
              1 null stderr
    atf_check 'grep "more than once" stderr' 0 ignore null
    rm -f fmt.out

    # Check that defining two outputs over stdout (but using different
    # paths) does not work.
    atf_check 'atf-report -o csv:- -o ticker:/dev/stdout <tps.out' \
              1 null stderr
    atf_check 'grep "more than once" stderr' 0 ignore null
    rm -f fmt.out
}

atf_test_case output_csv
output_csv_head()
{
    atf_set "descr" "Checks the CSV output format"
}
output_csv_body()
{
    create_helpers
    run_helpers

# NO_CHECK_STYLE_BEGIN
    cat >expout <<EOF
tc, dir1/tp1, main, passed
tp, dir1/tp1, passed
tc, dir1/tp2, main, failed, This always fails
tp, dir1/tp2, failed
tc, tp3, main, passed
tp, tp3, passed
tc, tp4, main, failed, This always fails
tp, tp4, failed
tp, tp5, bogus, There were errors parsing the output of the test program: Line 1: Unexpected token \`<<EOF>>'; expected a header name.
EOF
# NO_CHECK_STYLE_END

    atf_check 'atf-report -o csv:- <tps.out' 0 expout null
}

atf_test_case output_ticker
output_ticker_head()
{
    atf_set "descr" "Checks the ticker output format"
}
output_ticker_body()
{
    create_helpers
    run_helpers

# NO_CHECK_STYLE_BEGIN
    cat >expout <<EOF
dir1/tp1 (1/5): 1 test cases
    main: Passed.

dir1/tp2 (2/5): 1 test cases
    main: Failed: This always fails

tp3 (3/5): 1 test cases
    main: Passed.

tp4 (4/5): 1 test cases
    main: Failed: This always fails

tp5 (5/5): 0 test cases
tp5: BOGUS TEST PROGRAM: Cannot trust its results because of \`There were errors parsing the output of the test program: Line 1: Unexpected token \`<<EOF>>'; expected a header name.'

Failed (bogus) test programs:
    tp5

Failed test cases:
    dir1/tp2:main, tp4:main

Summary for 5 test programs:
    2 passed test cases.
    2 failed test cases.
    0 skipped test cases.
EOF

    atf_check 'atf-report -o ticker:- <tps.out' 0 expout null
}
# NO_CHECK_STYLE_END

atf_test_case output_xml
output_xml_head()
{
    atf_set "descr" "Checks the XML output format"
}
output_xml_body()
{
    create_helpers
    run_helpers

# NO_CHECK_STYLE_BEGIN
    cat >expout <<EOF
<?xml version="1.0"?>
<!DOCTYPE tests-results PUBLIC "-//NetBSD//DTD ATF Tests Results 0.1//EN" "http://www.NetBSD.org/XML/atf/tests-results.dtd">

<tests-results>
<info class="startinfo">A value</info>
<tp id="dir1/tp1">
<tc id="main">
<passed />
</tc>
</tp>
<tp id="dir1/tp2">
<tc id="main">
<failed>This always fails</failed>
</tc>
</tp>
<tp id="tp3">
<tc id="main">
<passed />
</tc>
</tp>
<tp id="tp4">
<tc id="main">
<failed>This always fails</failed>
</tc>
</tp>
<tp id="tp5">
<failed>There were errors parsing the output of the test program: Line 1: Unexpected token \`&lt;&lt;EOF&gt;&gt;'; expected a header name.</failed>
</tp>
<info class="endinfo">Another value</info>
</tests-results>
EOF
# NO_CHECK_STYLE_END

    atf_check 'atf-report -o xml:- <tps.out' 0 expout null
}

atf_test_case output_xml_space
output_xml_space_head()
{
    atf_set "descr" "Checks that the XML output format properly preserves" \
                    "leading and trailing whitespace in stdout and stderr" \
                    "lines"
}
output_xml_space_body()
{
    cp $(atf_get_srcdir)/h_misc .
    cat >Atffile <<EOF
Content-Type: application/X-atf-atffile; version="1"

prop: test-suite = atf

tp: h_misc
EOF

# NO_CHECK_STYLE_BEGIN
    cat >expout <<EOF
<?xml version="1.0"?>
<!DOCTYPE tests-results PUBLIC "-//NetBSD//DTD ATF Tests Results 0.1//EN" "http://www.NetBSD.org/XML/atf/tests-results.dtd">

<tests-results>
<info class="startinfo">A value</info>
<tp id="h_misc">
<tc id="atf_report_diff">
<so>--- a	2007-11-04 14:00:41.000000000 +0100</so>
<so>+++ b	2007-11-04 14:00:48.000000000 +0100</so>
<so>@@ -1,7 +1,7 @@</so>
<so> This test is meant to simulate a diff.</so>
<so> Blank space at beginning of context lines must be preserved.</so>
<so> </so>
<so>-First original line.</so>
<so>-Second original line.</so>
<so>+First modified line.</so>
<so>+Second modified line.</so>
<so> </so>
<so> EOF</so>
<passed />
</tc>
</tp>
<info class="endinfo">Another value</info>
</tests-results>
EOF
# NO_CHECK_STYLE_END

    TESTCASE=atf_report_diff; export TESTCASE
    run_helpers
    atf_check 'atf-report -o xml:- <tps.out' 0 expout null
}

atf_init_test_cases()
{
    atf_add_test_case default
    atf_add_test_case oflag
    atf_add_test_case output_csv
    atf_add_test_case output_ticker
    atf_add_test_case output_xml
    atf_add_test_case output_xml_space
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
