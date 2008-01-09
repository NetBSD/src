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

atf_test_case has
has_head()
{
    atf_set "descr" "Verifies that atf_config_has works"
}
has_body()
{
    atf_config_has "workdir" || atf_fail "Missing expected variable"
    atf_config_has "undefined" && atf_fail "Found unexpected variable"
}

atf_test_case get
get_head()
{
    atf_set "descr" "Verifies that atf_config_get works"
}
get_body()
{
    echo "Querying an undefined variable"
    ( atf_config_get "undefined" ) >out 2>err && \
        atf_fail "Getting an undefined variable succeeded"
    grep 'not find' err || \
        atf_fail "Getting an undefined variable did not report an error"

    echo "Querying an undefined variable using a default value"
    v=$(atf_config_get "undefined" "the default value")
    [ "${v}" = "the default value" ] || \
        atf_fail "Default value does not work"

    echo "Querying an known defined variable"
    v=$(atf_config_get "workdir")
    case "${v}" in
        /*)
            ;;

        *)
            atf_fail "Getting a known variable did not work"
            ;;
    esac
}

atf_init_test_cases()
{
    atf_add_test_case has
    atf_add_test_case get
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
