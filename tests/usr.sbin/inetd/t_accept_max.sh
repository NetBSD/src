#	$NetBSD: t_accept_max.sh,v 1.2 2026/05/17 01:31:55 riastradh Exp $
#
# Copyright (c) 2026 The NetBSD Foundation, Inc.
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

atf_test_case max2_pos
max2_pos_head()
{
	atf_set "descr" "Test inetd with accept_max of 2 (positional syntax)"
	atf_set "timeout" 10
}
max2_pos_body()
{
	cat <<EOF >inetd.conf
$(pwd)/sock stream,2 unix nowait $(whoami) $(pwd)/run run
EOF
	max2_test
}

atf_test_case max2_kv
max2_kv_head()
{
	atf_set "descr" "Test inetd with accept_max of 2 (k/v syntax)"
	atf_set "timeout" 10
}
max2_kv_body()
{
	cat <<EOF >inetd.conf
$(pwd)/sock on
	socktype = stream,
	protocol = unix,
	accept_max = 2,
	wait = no,
	user = $(whoami),
	exec = $(pwd)/run,
	args = run;
EOF
	max2_test
}

max2_test()
{
	atf_require_prog ${INETD:-inetd}
	(max2_subshell) || atf_fail "$?"
}
max2_subshell()
{

	# Save traps and arrange to kill jobs.  We have at most five
	# jobs running or pending at any given time, so job ids %0
	# through %4 should cover them all.
	#
	# XXX Hope atf doesn't have any background jobs of its own to
	# collide with the job ids!
	#
	traps=$(trap)
	trap 'eval "$traps"; kill -9 %0 %1 %2 %3 %4 2>/dev/null; wait' \
	    ALRM EXIT HUP INT PIPE TERM
	reset=$(set +o)
	set -e -m -x

	# Create some state.
	#
	echo 0 >ntasks
	: >output
	mkfifo 1A 1B 2A 2B 3A 3B 4A 4B

	# Configure inetd with a daemon.  The daemon will:
	#
	# 1. read a connection number $client,
	# 2. append ${client} to a log of output,
	# 3. write to fifo ${client}A to notify the test that it has
	#    done so,
	# 4. and then read from fifo ${client}B before exiting.
	#
	cat <<'EOF' >run
#!/bin/sh
set -eu -o pipefail
flock ntasks sh -c 'n=$(cat ntasks); echo $((n + 1)) >ntasks'
read -r client
printf '%s\n' "$client" >>output
: >"${client}A"
: <"${client}B"
flock ntasks sh -c 'n=$(cat ntasks); echo $((n - 1)) >ntasks'
EOF
	chmod +x run

	# Start inetd and wait for it to bind its sockets.
	#
	${INETD:-inetd} -d -f ./inetd.conf & inetd=$!
	echo inetd running as $inetd
	sleep 1

	# Nothing should have happened so far, and no tasks should be
	# running:
	#
	case $(cat ntasks):$(cat output) in
	0:)	;;
	*)	echo '# ntasks' >&2
		cat ntasks >&2
		echo '# output' >&2
		cat output >&2
		return 1
		;;
	esac
	: >output

	# Open connection 1, and wait for the server to acknowledge it:
	#
	timeout 10s sh -c 'echo 1 | nc -U sock' & client1=$!
	echo client 1 running as $client1
	timeout 10s sh -c ': <1A' || return $?
	case $(cat ntasks):$(cat output) in
	1:1)
		;;
	*)	echo '# ntasks' >&2
		cat ntasks >&2
		echo '# output' >&2
		cat output >&2
		return 1
		;;
	esac
	: >output

	# After this, the server should hang until we open 1B for
	# write.
	#
	# Open client 2 and wait for the server to acknowledge it too.
	#
	timeout 10s sh -c 'echo 2 | nc -U sock' & client2=$!
	echo client 2 running as $client2
	timeout 10s sh -c ': <2A' || return $?
	case $(cat ntasks):$(cat output) in
	2:2)
		;;
	*)	echo '# ntasks' >&2
		cat ntasks >&2
		echo '# output' >&2
		cat output >&2
		return 1
		;;
	esac
	: >output

	# Open client 3, and wait a second for it to answer -- it
	# shouldn't answer or increment ntasks, because, having hit the
	# accept_max, inetd should have suspended it:
	#
	timeout 10s sh -c 'echo 3 | nc -U sock' & client3=$!
	echo client 3 running as $client3
	sleep 1

	# Two tasks should still be running at this point, but no new
	# output should have happened:
	#
	case $(cat ntasks):$(cat output) in
	2:)
		;;
	*)	echo '# ntasks' >&2
		cat ntasks >&2
		echo '# output' >&2
		cat output >&2
		return 1
		;;
	esac
	: >output

	# Now let the connection 1 finish by writing to 1B, and wait
	# for the server to acknowledge client 3, by reading from 3A --
	# it should now print 3, promptly, and the first connection
	# should finish promptly too:
	#
	timeout 10s sh -c ': >1B; : <3A' || return $?
	case $(cat ntasks):$(cat output) in
	2:3)
		;;
	*)	echo '# ntasks' >&2
		cat ntasks >&2
		echo '# output' >&2
		cat output >&2
		return 1
		;;
	esac
	: >output
	wait $client1 || atf_fail "failed to wait for client 1 ($?)"
	case $(cat ntasks):$(cat output) in
	2:)
		;;
	*)	echo '# ntasks' >&2
		cat ntasks >&2
		echo '# output' >&2
		cat output >&2
		return 1
		;;
	esac
	: >output

	# Finally, let connections 2 and 3 finish, and wait for them to
	# do so:
	#
	timeout 10s sh -c ': >2B; : >3B' || return $?
	wait $client2 || atf_fail "failed to wait for client 2 ($?)"
	wait $client3 || atf_fail "failed to wait for client 3 ($?)"

	# No more tasks should be running, and no more output should have
	# happened.
	#
	case $(cat ntasks):$(cat output) in
	0:)
		;;
	*)	echo '# ntasks' >&2
		cat ntasks >&2
		echo '# output' >&2
		cat output >&2
		return 1
		;;
	esac
	: >output

	# All done; kill inetd, wait for it to exit, and restore traps:
	#
	kill $inetd; wait $inetd
	eval "$reset"
	eval "$traps"
}

atf_init_test_cases()
{

	atf_add_test_case max2_kv
	atf_add_test_case max2_pos
}
