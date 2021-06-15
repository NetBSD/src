#	$NetBSD: $
#
# Copyright (c) 2021 Internet Initiative Japan Inc.
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
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

DEBUG=${DEBUG:-false}
SOCK=unix://commsock
HIJACKING="env LD_PRELOAD=/usr/lib/librumphijack.so \
    RUMPHIJACK=path=/rump,socket=all:nolocal,sysctl=yes"

atf_sysctl_rd()
{
	local sysctl_key=$1
	local expected=$2

	atf_check -s exit:0 -o match:"$expected" \
	    rump.sysctl -n $sysctl_key
}

atf_sysctl_wr()
{
	local sysctl_key=$1
	local value=$2

	atf_check -s exit:0 -o ignore rump.sysctl -w $sysctl_key=$value
}

atf_sysctl_wait()
{
	local sysctl_key=$1
	local expected=$2
	local n=10
	local i
	local v

	for i in $(seq $n); do
		v=$(rump.sysctl -n $sysctl_key)
		if [ x"$v" = x"$expected" ]; then
			return
		fi
		sleep 0.5
	done

	atf_fail "Couldn't get the value for $n seconds."
}

atf_test_case simplehook_basic cleanup
simplehook_basic_head()
{

	atf_set "descr" "tests for basically functions of simplehook"
	atf_set "require.progs" "rump_server"
}


simplehook_basic_body()
{
	local key_hklist="kern.simplehook_tester.hook_list"
	local key_hk0="kern.simplehook_tester.hook0"
	local key_hk1="kern.simplehook_tester.hook1"

	rump_server -lrumpkern_simplehook_tester $SOCK

	export RUMP_SERVER=$SOCK

	$DEBUG && rump.sysctl -e kern.simplehook_tester
	atf_check -s exit:0 -o ignore rump.sysctl -e kern.simplehook_tester

	# create and destroy
	atf_sysctl_rd ${key_hklist}.created '0'
	atf_sysctl_wr ${key_hklist}.created '1'
	atf_sysctl_wr ${key_hklist}.created '0'
	$DEBUG && rump.sysctl -e kern.simplehook_tester

	# establish one hook
	atf_sysctl_wr ${key_hklist}.created '1'
	atf_sysctl_wr ${key_hk0}.established '1'
	atf_sysctl_wr ${key_hk0}.count '0'
	atf_sysctl_wr ${key_hklist}.dohooks '1'
	atf_sysctl_wr ${key_hklist}.dohooks '0'
	atf_sysctl_rd ${key_hk0}.count '1'
	atf_sysctl_wr ${key_hk0}.established '0'
	atf_sysctl_wr ${key_hklist}.created '0'

	# establish two hooks
	atf_sysctl_wr ${key_hklist}.created '1'
	atf_sysctl_wr ${key_hk0}.established '1'
	atf_sysctl_wr ${key_hk1}.established '1'
	atf_sysctl_wr ${key_hk0}.count '0'
	atf_sysctl_wr ${key_hk1}.count '0'

	atf_sysctl_wr ${key_hklist}.dohooks '1'
	atf_sysctl_wr ${key_hklist}.dohooks '0'
	atf_sysctl_rd ${key_hk0}.count '1'
	atf_sysctl_rd ${key_hk1}.count '1'

	atf_sysctl_wr ${key_hk0}.established '0'
	atf_sysctl_wr ${key_hk1}.established '0'
	atf_sysctl_wr ${key_hklist}.created '0'
}

simplehook_basic_cleanup()
{
	export RUMP_SERVER=$SOCK

	$DEBUG && rump.sysctl -e kern.simplehook_tester
	$DEBUG && $HIJACKING dmesg
	rump.halt
}

atf_test_case simplehook_disestablish cleanup
simplehook_disestablish_head()
{

	atf_set "descr" "tests for disestablish of simplehook"
	atf_set "require.progs" "rump_server"
}

simplehook_disestablish_body()
{
	local key_hklist="kern.simplehook_tester.hook_list"
	local key_hk0="kern.simplehook_tester.hook0"
	local key_hk1="kern.simplehook_tester.hook1"

	rump_server -lrumpkern_simplehook_tester $SOCK

	export RUMP_SERVER=$SOCK

	$DEBUG && rump.sysctl -e kern.simplehook_tester
	atf_check -s exit:0 -o ignore rump.sysctl -e kern.simplehook_tester

	#
	# disestablish on the running hook
	#
	atf_sysctl_wr ${key_hklist}.created '1'
	atf_sysctl_wr ${key_hk0}.established '1'
	atf_sysctl_wr ${key_hk0}.disestablish_in_hook '1'
	atf_sysctl_wr ${key_hklist}.dohooks '1'
	atf_sysctl_wr ${key_hklist}.dohooks '0'

	# already disestablished
	atf_sysctl_rd ${key_hk0}.established '0'
	atf_sysctl_wr ${key_hk0}.disestablish_in_hook '0'

	atf_sysctl_wr ${key_hklist}.created '0'

	#
	# disestablish called hook while doing other hook
	#
	atf_sysctl_wr ${key_hklist}.created '1'
	atf_sysctl_wr ${key_hk0}.established '1'
	atf_sysctl_wr ${key_hk1}.established '1'

	atf_sysctl_wr ${key_hk0}.count '0'
	atf_sysctl_wr ${key_hk1}.count '0'
	atf_sysctl_wr ${key_hk0}.stopping '1'

	# calls hook1 -> hook0
	atf_sysctl_wr ${key_hklist}.dohooks '1'

	# stop in hook0
	atf_sysctl_wait ${key_hk0}.stopped '1'

	atf_sysctl_rd ${key_hk1}.count '1'
	atf_sysctl_wr ${key_hk1}.established '0'

	# wakeup hook0
	atf_sysctl_wr ${key_hk0}.stopping '0'
	atf_sysctl_wr ${key_hklist}.dohooks '0'

	atf_sysctl_wr ${key_hk0}.established '0'
	atf_sysctl_wr ${key_hklist}.created '0'

	#
	# disestablish a hook in running hook list
	#
	atf_sysctl_wr ${key_hklist}.created '1'
	atf_sysctl_wr ${key_hk0}.established '1'
	atf_sysctl_wr ${key_hk1}.established '1'

	atf_sysctl_wr ${key_hk0}.count '0'
	atf_sysctl_wr ${key_hk1}.count '0'
	atf_sysctl_wr ${key_hk1}.stopping '1'

	# calls hook1 -> hook0
	atf_sysctl_wr ${key_hklist}.dohooks '1'

	# stop in hook1
	atf_sysctl_wait ${key_hk1}.stopped '1'

	atf_sysctl_wr ${key_hk0}.established '0'

	# wakeup hook1
	atf_sysctl_wr ${key_hk1}.stopping '0'
	atf_sysctl_wr ${key_hklist}.dohooks '0'

	# hook0 is not called
	atf_sysctl_rd ${key_hk0}.count '0'

	atf_sysctl_wr ${key_hk1}.established '0'
	atf_sysctl_wr ${key_hklist}.created '0'

	#
	# disestablish the running hook
	#
	atf_sysctl_wr ${key_hklist}.created '1'
	atf_sysctl_wr ${key_hk0}.established '1'
	atf_sysctl_wr ${key_hk0}.stopping '1'

	atf_sysctl_wr ${key_hklist}.dohooks '1'

	atf_sysctl_wait ${key_hk0}.stopped '1'
	atf_sysctl_wr ${key_hk0}.established '0'

	atf_sysctl_wr ${key_hklist}.dohooks '0'
	atf_sysctl_wr ${key_hklist}.created '0'
}

simplehook_disestablish_cleanup()
{
	export RUMP_SERVER=$SOCK

	$DEBUG && rump.sysctl -e kern.simplehook_tester
	$DEBUG && $HIJACKING dmesg
	rump.halt
}

atf_test_case simplehook_nolock cleanup
simplehook_nolock_head()
{

	atf_set "descr" "tests for hook that does not use lock in it"
	atf_set "require.progs" "rump_server"
}

simplehook_nolock_body()
{
	local key_hklist="kern.simplehook_tester.hook_list"
	local key_hk="kern.simplehook_tester.nbhook"

	rump_server -lrumpkern_simplehook_tester $SOCK

	export RUMP_SERVER=$SOCK
	$DEBUG && rump.sysctl -e kern.simplehook_tester
	atf_check -s exit:0 -o ignore rump.sysctl -e kern.simplehook_tester

	atf_sysctl_wr ${key_hklist}.created '1'
	atf_sysctl_wr ${key_hk}.established '1'

	atf_sysctl_wr ${key_hklist}.dohooks '1'
	atf_sysctl_wr ${key_hk}.established '0'
	atf_sysctl_wr ${key_hklist}.dohooks '0'

	atf_sysctl_wr ${key_hklist}.created '0'
}

simplehook_nolock_cleanup()
{
	export RUMP_SERVER=$SOCK

	$DEBUG && rump.sysctl -e kern.simplehook_tester
	$DEBUG && $HIJACKING dmesg
	rump.halt
}

atf_init_test_cases()
{

	atf_add_test_case simplehook_basic
	atf_add_test_case simplehook_disestablish
	atf_add_test_case simplehook_nolock
}
