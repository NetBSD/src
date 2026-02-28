#	$NetBSD: t_ipc.sh,v 1.1 2026/02/28 20:43:28 riastradh Exp $
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

setup()
{
	. $(atf_get_srcdir)/../h_funcs.subr
	require_fs null

	atf_check mkdir mountfrom
	atf_check mkdir mountpoint

	atf_check mount -t null "$@" "$(pwd)/mountfrom" "$(pwd)/mountpoint"
}

teardown()
{
	umount -f -R "$(pwd)/mountpoint"
}

atf_test_case fifo_write cleanup
fifo_write_head()
{
	atf_set "descr" "Tests writing to a fifo through a null mount"
	atf_set "require.user" "root"
}
fifo_write_body()
{
	setup
	atf_check mkfifo mountfrom/fifo
	timeout 5s sh -c 'cat <mountfrom/fifo >output' &
	reader=$!
	timeout 5s sh -c 'printf hello >mountpoint/fifo'
	wait $reader
	atf_check -o inline:hello cat output
}
fifo_write_cleanup()
{
	teardown
}

atf_test_case fifo_read cleanup
fifo_read_head()
{
	atf_set "descr" "Tests reading from a fifo through a null mount"
	atf_set "require.user" "root"
}
fifo_read_body()
{
	setup
	atf_check mkfifo mountfrom/fifo
	timeout 5s sh -c 'printf hello >mountfrom/fifo' &
	writer=$!
	atf_check -o inline:hello timeout 5s cat mountpoint/fifo
	kill $writer
	wait $writer
}
fifo_read_cleanup()
{
	teardown
}

atf_test_case socket_stream_connect cleanup
socket_stream_connect_head()
{
	atf_set "descr" "Tests connecting a stream socket through null mount"
	atf_set "require.user" "root"
}
socket_stream_connect_body()
{
	setup
	printf hello | timeout 5s nc -N -U -l mountfrom/socket >output &
	listener=$!
	atf_expect_fail "PR kern/51963:" \
	    "sockets in chroot sandbox via null-mounts don't work"
	status=0
	sleep 1                 # wait for nc to bind and listen, ugh
	output=$(printf world | nc -N -U mountpoint/socket) || status=$?
	case $status,$output in
	0,hello)
		wait $listener
		;;
	*)	kill $listener
		wait $listener
		atf_fail nc: status=$status output="$output"
		;;
	esac
	atf_check -o inline:world cat output
}
socket_stream_connect_cleanup()
{
	teardown
}

atf_test_case socket_stream_listen cleanup
socket_stream_listen_head()
{
	atf_set "descr" "Tests stream socket listening through null mount"
	atf_set "require.user" "root"
}
socket_stream_listen_body()
{
	setup
	printf hello | timeout 5s nc -N -U -l mountpoint/socket >output &
	listener=$!
	atf_expect_fail "PR kern/51963:" \
	    "sockets in chroot sandbox via null-mounts don't work"
	status=0
	sleep 1                 # wait for nc to bind and listen, ugh
	output=$(printf world | nc -N -U mountfrom/socket) || status=$?
	case $status,$output in
	0,hello)
		wait $listener
		;;
	*)	kill $listener
		wait $listener
		atf_fail nc: status=$status output="$output"
		;;
	esac
	atf_check -o inline:world cat output
}
socket_stream_listen_cleanup()
{
	teardown
}

atf_init_test_cases()
{
	atf_add_test_case fifo_read
	atf_add_test_case fifo_write
	atf_add_test_case socket_stream_connect
	atf_add_test_case socket_stream_listen
}
