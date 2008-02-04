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
    atf_check "${atf_exec} ./helper.sh" 0 expout null
    atf_check "${atf_exec} -- ./helper.sh" 0 expout null

    echo 'arg1' >expout
    atf_check "${atf_exec} ./helper.sh arg1" 0 expout null
    atf_check "${atf_exec} -- ./helper.sh arg1" 0 expout null

    echo 'arg1 arg2' >expout
    atf_check "${atf_exec} ./helper.sh arg1 arg2" 0 expout null
    atf_check "${atf_exec} -- ./helper.sh arg1 arg2" 0 expout null
}

pgid_of()
{
    ps -o pgid -p ${1} | tail -n 1
}

atf_test_case process_group
process_group_head()
{
    atf_set "descr" "Checks that giving -g creates a new process group"
}
process_group_body()
{
    cat >helper.sh <<EOF
#! $(atf-config -t atf_shell)

touch ready
while [ ! -f done ]; do sleep 1; done
EOF
    chmod +x helper.sh

    this_pgid=$(pgid_of ${$})

    echo "Checking that the lack of -g does not change the process group"
    rm -f ready done
    ${atf_exec} ./helper.sh &
    while [ ! -f ready ]; do sleep 1; done
    child_pgid=$(pgid_of ${!})
    touch done
    wait ${!}

    echo "My PGID is ${this_pgid}"
    echo "atf-exec's PGID is ${child_pgid}"
    [ ${this_pgid} -eq ${child_pgid} ] || \
        atf_fail "Process group was changed"

    echo "Checking that giving -g changes the process group"
    rm -f ready done
    ${atf_exec} -g ./helper.sh &
    while [ ! -f ready ]; do sleep 1; done
    child_pgid=$(pgid_of ${!})
    touch done
    wait ${!}

    echo "My PGID is ${this_pgid}"
    echo "atf-exec's PGID is ${child_pgid}"
    [ ${this_pgid} -ne ${child_pgid} ] || \
        atf_fail "Process group was not changed"
}

atf_init_test_cases()
{
    atf_add_test_case passthrough
    atf_add_test_case process_group
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
