#	$NetBSD: t_tcp_nc.sh,v 1.1 2022/11/02 09:37:56 ozaki-r Exp $
#
# Copyright (c) 2022 Internet Initiative Japan Inc.
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

SOCK_LOCAL=unix://tcp_nc_local
SOCK_PEER=unix://tcp_nc_peer
BUS=./bus_tcp_nc
FILE_SND=./file.snd
FILE_RCV=./file.rcv
IP_LOCAL=10.0.0.1
IP_PEER=10.0.0.2
NC_OPTS="-w 3"

DEBUG=${DEBUG:-false}

setup_servers()
{

	rump_server_start $SOCK_LOCAL
	rump_server_start $SOCK_PEER
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_PEER shmif0 $BUS
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.ifconfig shmif0 $IP_LOCAL/24
	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 rump.ifconfig shmif0 $IP_PEER/24
}

prepare_snd_file()
{
	local file=$FILE_SND
	local size=${1:-5120}

	# XXX accepts only multiple of 512
	dd if=/dev/zero of=$file bs=512 count=$(($size / 512))
}

cleanup_file()
{

	rm -f $FILE_SND
	rm -f $FILE_RCV
}

test_tcp_nc_econnrefused()
{

	export RUMP_SERVER=$SOCK_LOCAL

	atf_check -s not-exit:0 -e match:'Connection refused' \
	    $HIJACKING nc $NC_OPTS -v 127.0.0.1 80 < $FILE_SND
	atf_check -s not-exit:0 -e match:'Connection refused' \
	    $HIJACKING nc $NC_OPTS -v $IP_PEER 80 < $FILE_SND
}

test_tcp_nc_rcvbuf()
{

	start_nc_server $SOCK_PEER 80 $FILE_RCV ipv4 "-O 16384"

	atf_check -s exit:0 $HIJACKING nc $NC_OPTS $IP_PEER 80 < $FILE_SND
}

test_tcp_nc_small_rcvbuf()
{

	start_nc_server $SOCK_PEER 80 $FILE_RCV ipv4 "-O 1024"

	prepare_snd_file $((512 * 1024))
	atf_check -s exit:0 $HIJACKING nc $NC_OPTS $IP_PEER 80 < $FILE_SND
}

test_tcp_nc_sndbuf()
{

	start_nc_server $SOCK_PEER 80 $FILE_RCV ipv4

	atf_check -s exit:0 $HIJACKING nc $NC_OPTS -I 16384 $IP_PEER 80 < $FILE_SND
}

test_tcp_nc_small_sndbuf()
{

	start_nc_server $SOCK_PEER 80 $FILE_RCV ipv4

	prepare_snd_file $((512 * 1024))
	atf_check -s exit:0 $HIJACKING nc $NC_OPTS -I 1024 $IP_PEER 80 < $FILE_SND
}

test_tcp_nc_md5sig()
{

	atf_expect_fail "TCP_SIGNATURE is not enabled by default"

	start_nc_server $SOCK_PEER 80 $FILE_RCV ipv4

	atf_check -s exit:0 $HIJACKING nc $NC_OPTS -S $IP_PEER 80 < $FILE_SND
}

add_test_tcp_nc()
{
	local type=$1
	local name= desc=

	desc="Test TCP with nc ($type)"
	name="tcp_nc_${type}"

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server nc
	    }
	    ${name}_body() {
	        setup_servers
	        prepare_snd_file
	        test_tcp_nc_$type
	        rump_server_destroy_ifaces
	    }
	    ${name}_cleanup() {
	        cleanup_file
	        \$DEBUG && dump
	        cleanup
	    }
	"
	atf_add_test_case ${name}
}

atf_init_test_cases()
{

	add_test_tcp_nc econnrefused
	add_test_tcp_nc rcvbuf
	add_test_tcp_nc small_rcvbuf
	add_test_tcp_nc sndbuf
	add_test_tcp_nc small_sndbuf
	add_test_tcp_nc md5sig
}
