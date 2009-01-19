#
# Automated Testing Framework (atf)
#
# Copyright (c) 2008 The NetBSD Foundation, Inc.
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

atf_exec=$(atf-config -t atf_libexecdir)/atf-exec

atf_test_case passthrough
passthrough_head()
{
    atf_set "descr" "Ensures that this executes the given command if" \
                    "no options are provided"
}
passthrough_body()
{
    cat >helper.sh <<EOF
#! $(atf-config -t atf_shell)
echo "\${@}"
EOF
    chmod +x helper.sh

    echo '' >expout
    atf_check -s eq:0 -o file:expout -e empty ${atf_exec} ./helper.sh
    atf_check -s eq:0 -o file:expout -e empty ${atf_exec} -- ./helper.sh

    echo 'arg1' >expout
    atf_check -s eq:0 -o file:expout -e empty ${atf_exec} ./helper.sh arg1
    atf_check -s eq:0 -o file:expout -e empty ${atf_exec} -- ./helper.sh arg1

    echo 'arg1 arg2' >expout
    atf_check -s eq:0 -o file:expout -e empty \
              ${atf_exec} ./helper.sh arg1 arg2
    atf_check -s eq:0 -o file:expout -e empty \
              ${atf_exec} -- ./helper.sh arg1 arg2
}

atf_test_case timeout_syntax
timeout_syntax_head()
{
    atf_set "descr" "Checks syntax validation for -t"
}
timeout_syntax_body()
{
    atf_check -s eq:1 -o empty -e save:stderr ${atf_exec} -t '' true
    atf_check -s eq:0 -o ignore -e empty grep 'Invalid.*secs:file' stderr

    atf_check -s eq:1 -o empty -e save:stderr ${atf_exec} -t ':cookie' true
    atf_check -s eq:0 -o ignore -e empty grep 'Invalid.*secs.*empty' stderr

    atf_check -s eq:1 -o empty -e save:stderr ${atf_exec} -t '123:' true
    atf_check -s eq:0 -o ignore -e empty grep 'Invalid.*file.*empty' stderr

    atf_check -s eq:1 -o empty -e save:stderr ${atf_exec} -t 'foo:bar' true
    atf_check -s eq:0 -o ignore -e empty grep 'convert.*string' stderr
}

try_timeout()
{
    to=${1}; delay=${2}
    rm -f cookie
    if [ ${to} -lt ${delay} -a ${to} -gt 0 ]; then
        atf_check -s eq:1 -o empty -e empty \
                  ${atf_exec} -t "${to}:cookie" sleep ${delay}
        test -f cookie || \
            atf_fail "Didn't find the timeout cookie but it should be there"
    else
        atf_check -s eq:0 -o empty -e empty \
                  ${atf_exec} -t "${to}:cookie" sleep ${delay}
        test -f cookie && \
            atf_fail "Found the timeout cookie but it shouldn't be there"
    fi
}

atf_test_case timeout
timeout_head()
{
    atf_set "descr" "Checks that commands are killed if their timeout" \
                    "expires"
}
timeout_body()
{
    try_timeout 1 5
    try_timeout 5 1

    try_timeout 0 1
}

atf_init_test_cases()
{
    atf_add_test_case passthrough
    atf_add_test_case timeout_syntax
    atf_add_test_case timeout
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
