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
atf_killpg=$(atf-config -t atf_libexecdir)/atf-killpg

atf_test_case default
default_head()
{
    atf_set "descr" "Verifies that the signal sent by default is correct"
}
default_body()
{
    cat >helper.sh <<EOF
#! $(atf-config -t atf_shell)
trap 'touch sigterm; exit 0' TERM
touch waiting
while test ! -f done; do sleep 1; done
EOF
    chmod +x helper.sh

    ${atf_exec} -g ./helper.sh >stdout &
    while test ! -f waiting; do sleep 1; done
    ${atf_killpg} ${!}
    while test ! -f sigterm; do sleep 1; done
    touch done
    wait ${!}

    atf_check 'test -f sigterm' 0 null null
}

atf_test_case sflag
sflag_head()
{
    atf_set "descr" "Verifies that the -s flag correctly changes the" \
                    "signal sent to processes"
}
sflag_body()
{
    cat >helper.sh <<EOF
#! $(atf-config -t atf_shell)
trap 'touch sighup; exit 0' HUP
trap 'touch sigterm; exit 0' TERM
touch waiting
while test ! -f done; do sleep 1; done
EOF
    chmod +x helper.sh

    ${atf_exec} -g ./helper.sh >stdout &
    while test ! -f waiting; do sleep 1; done
    ${atf_killpg} -s 1 ${!}
    while test ! -f sighup -a ! -f sigterm; do sleep 1; done
    touch done
    wait ${!}

    atf_check 'test -f sighup' 0 null null
    atf_check 'test -f sigterm' 1 null null
}

atf_test_case group
group_head()
{
    atf_set "descr" "Verifies that the signal is sent to the whole group"
}
group_body()
{
    cat >helper.sh <<EOF
#! $(atf-config -t atf_shell)
if [ \${#} -eq 1 ]; then
    ./helper.sh &
    trap 'touch sig1' HUP
    touch waiting1
    echo "Process 1 waiting for termination"
    while [ ! -f done ]; do sleep 1; done
    echo "Process 1 waiting for process 2"
    wait \${!}
    echo "Process 1 terminating"
    touch done1
else
    trap 'touch sig2' HUP
    touch waiting2
    echo "Process 2 waiting for termination"
    while [ ! -f done ]; do sleep 1; done
    echo "Process 2 terminating"
    touch done2
fi
EOF
    chmod +x helper.sh

    ${atf_exec} -g ./helper.sh initial &
    echo "Waiting for process 1 to be alive"
    while test ! -f waiting1; do sleep 1; done
    echo "Waiting for process 2 to be alive"
    while test ! -f waiting2; do sleep 1; done
    echo "Sending signal"
    ${atf_killpg} -s 1 ${!}
    echo "Waiting for termination"
    touch done
    while test ! -f done1 -a ! -f done2; do sleep 1; done
    wait

    atf_check 'test -f sig1' 0 null null
    atf_check 'test -f sig2' 0 null null
}

atf_init_test_cases()
{
    atf_add_test_case default
    atf_add_test_case sflag
    atf_add_test_case group
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
