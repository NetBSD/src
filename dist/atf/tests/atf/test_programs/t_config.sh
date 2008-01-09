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

atf_test_case vflag
vflag_head()
{
    atf_set "descr" "Tests that the -v flag works correctly to set" \
                    "configuration variables"
}
vflag_body()
{
    srcdir=$(atf_get_srcdir)
    h_cpp=${srcdir}/h_cpp
    h_sh=${srcdir}/h_sh

    for h in ${h_cpp} ${h_sh}; do
        atf_check "${h} -s ${srcdir} -r3 \
                   config_unset 3>resout" 0 ignore ignore
        atf_check "grep 'passed' resout" 0 ignore null

        atf_check "${h} -s ${srcdir} -r3 -v 'test=' \
                   config_empty 3>resout" 0 ignore ignore
        atf_check "grep 'passed' resout" 0 ignore null

        atf_check "${h} -s ${srcdir} -r3 -v 'test=foo' \
                   config_value 3>resout" 0 ignore ignore
        atf_check "grep 'passed' resout" 0 ignore null

        atf_check "${h} -s ${srcdir} -r3 -v 'test=foo bar' \
                   config_multi_value 3>resout" 0 ignore ignore
        atf_check "grep 'passed' resout" 0 ignore null
    done
}

atf_init_test_cases()
{
    atf_add_test_case vflag
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
