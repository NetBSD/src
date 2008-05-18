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

atf_test_case newlines
newlines_head()
{
    atf_set "descr" "Tests that newlines provided as part of status'" \
                    "reasons are handled properly"
}
newlines_body()
{
    for h in $(get_helpers); do
        case ${h} in
            *h_sh*)
                # XXX Not implemented.
                continue
                ;;
        esac

        # NO_CHECK_STYLE_BEGIN
        cat >expout <<EOF
Content-Type: application/X-atf-tcs; version="1"

tcs-count: 1
tc-start: status_newlines_fail
tc-end: status_newlines_fail, failed, BOGUS REASON (THE ORIGINAL HAD NEWLINES): First line<<NEWLINE>>Second line
EOF
        # NO_CHECK_STYLE_END
        atf_check "${h} -s $(atf_get_srcdir) status_newlines_fail" \
                  1 expout null

        # NO_CHECK_STYLE_BEGIN
        cat >expout <<EOF
Content-Type: application/X-atf-tcs; version="1"

tcs-count: 1
tc-start: status_newlines_skip
tc-end: status_newlines_skip, skipped, BOGUS REASON (THE ORIGINAL HAD NEWLINES): First line<<NEWLINE>>Second line
EOF
        # NO_CHECK_STYLE_END
        atf_check "${h} -s $(atf_get_srcdir) status_newlines_skip" \
                  0 expout null
    done
}

atf_init_test_cases()
{
    atf_add_test_case newlines
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
