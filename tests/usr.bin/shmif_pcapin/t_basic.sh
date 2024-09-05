#	$NetBSD: t_basic.sh,v 1.1.2.2 2024/09/05 09:22:43 martin Exp $
#
# Copyright (c) 2017-2018 Internet Initiative Japan Inc.
# Copyright (c) 2011 The NetBSD Foundation, Inc.
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

SOCK=unix://commsock
BUS=bus0

unpack_file()
{

	atf_check -s exit:0 uudecode $(atf_get_srcdir)/${1}.bz2.uue
	atf_check -s exit:0 bunzip2 -f ${1}.bz2
}

atf_test_case pcap cleanup

pcap_head()
{

	atf_set "descr" "Write frames from pcap(3) file to shmif(4) interface"
}

pcap_body()
{
	unpack_file d_pcap.in
	unpack_file d_pcap.out

	rump_server_npf_start ${SOCK} # need librumpdev_bpf
	rump_server_add_iface ${SOCK} shmif0 ${BUS}

	export RUMP_SERVER=${SOCK}
	export LD_PRELOAD=/usr/lib/librumphijack.so
	export RUMPHIJACK=path=/rump,socket=all:nolocal,sysctl=yes,,blanket=/dev/bpf

	atf_check -s exit:0 rump.ifconfig shmif0 up

	# Capture frames on shmif0 to examine later.
	tcpdump -c 58 -eni shmif0 -w shmif0.in.pcap &
	sleep 1 # give shmif0 a change to turn into promiscuous mode
	# Write frames to the bus.
	atf_check -s exit:0 -o ignore shmif_pcapin d_pcap.in ${BUS}
	wait # for tcpdump to exit

	# Check if written frames surely arrives at shmif0.
	atf_check -s exit:0 -o match:"input: 58 packets, 5684 bytes" rump.ifconfig -v shmif0
	# Check if frames captured on shmif0 are expected ones.
	atf_check -s exit:0 -o file:d_pcap.out -e ignore tcpdump -entr shmif0.in.pcap

	unset LD_PRELOAD
	unset RUMP_SERVER
}

pcap_cleanup()
{

	$DEBUG && dump
	cleanup
}


atf_init_test_cases()
{

	atf_add_test_case pcap
}
