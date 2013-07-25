# $NetBSD: t_ldp_static.sh,v 1.1 2013/07/25 14:28:29 kefren Exp $
#
# Copyright (c) 2013 The NetBSD Foundation, Inc.
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

# IP/MPLS & LDP test
# Create 5 routers connected like this: R1--R2--R3--R4--R5--
# The goal is to push packets from R1 to the R5 shmif1 (the right one) interface
# Disable IP forwarding and enable MPLS forwarding on R2, R3 and R4
# Put some load on ldp by creating 100 IP routes from R1 to R5 on R[1,2,3,4]
# Start ldpd and wait for adjancencies to come up
# ping random addresses from R1 to R5
# As IP forwarding is disabled the packets should be
#    encapsulated into MPLS, use MPLS forwarding and decapsulated back into
#    IP when reaching target

MAX_B=10
MAX_C=10

RUMP_SERVER1=unix://./r1
RUMP_SERVER2=unix://./r2
RUMP_SERVER3=unix://./r3
RUMP_SERVER4=unix://./r4
RUMP_SERVER5=unix://./r5

RUMP_LIBS="-lrumpnet -lrumpnet_net -lrumpnet_netmpls -lrumpnet_netinet -lrumpnet_shmif"
LDP_FLAGS=""

atf_test_case ldp_static cleanup
ldp_static_head() {

	atf_set "descr" "IP/MPLS forwarding test using LDP and static IP routes"
	atf_set "require.progs" "rump_server"
	atf_set "use.fs" "true"
}

create_routes() {

	for b in `seq 1 $MAX_B`
		do for c in `seq 0 $MAX_C`
			do atf_check -s exit:0 \
				rump.route -q add 10.$b.$c.0/24 $1
		done
	done
}

delete_routes() {

	for b in `seq 1 $MAX_B`
		do for c in `seq 0 $MAX_C`
			do atf_check -s exit:0 \
				rump.route -q delete 10.$b.$c.0/24
		done
	done
}

ping_one() {

	# Add address on R5
	RUMP_SERVER=${RUMP_SERVER5} atf_check -s exit:0 \
		rump.ifconfig shmif1 $1/24 alias

	# Now ldpd on R5 should take notice of the new route and announce it
	# to R4's ldpd. ldpd on R4 should verify that the next hop
	# corresponds to its routing table and tag the route to an
	RUMP_SERVER=${RUMP_SERVER1} atf_check -s exit:0 -o ignore -e ignore \
		rump.ping -n -o -w 2 $1

	# Tear it down and bye bye
	RUMP_SERVER=${RUMP_SERVER5} atf_check -s exit:0 \
		rump.ifconfig shmif1 $1/24 -alias
}

create_servers() {

	# allows us to run as normal user
	ulimit -r 400

	atf_check -s exit:0 rump_server ${RUMP_LIBS} ${RUMP_SERVER1}
	atf_check -s exit:0 rump_server ${RUMP_LIBS} ${RUMP_SERVER2}
	atf_check -s exit:0 rump_server ${RUMP_LIBS} ${RUMP_SERVER3}
	atf_check -s exit:0 rump_server ${RUMP_LIBS} ${RUMP_SERVER4}
	atf_check -s exit:0 rump_server ${RUMP_LIBS} ${RUMP_SERVER5}

	# LDP HIJACK
	export RUMPHIJACK=path=/rump,socket=all,sysctl=yes
	export LD_PRELOAD=/usr/lib/librumphijack.so

	# Setup first server
	export RUMP_SERVER=${RUMP_SERVER1}
	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig shmif0 linkstr ./shdom1
	atf_check -s exit:0 rump.ifconfig shmif0 10.0.1.1/24
	atf_check -s exit:0 rump.ifconfig mpls0 create up
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.forwarding=0
	create_routes 10.0.1.2
	atf_check -s exit:0 rump.route -q add 10.0.5.0/24 10.0.1.2
	atf_check -s exit:0 /usr/sbin/ldpd ${LDP_FLAGS}

	# Setup second server
	export RUMP_SERVER=${RUMP_SERVER2}
	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig shmif0 linkstr ./shdom1
	atf_check -s exit:0 rump.ifconfig shmif0 10.0.1.2/24
	atf_check -s exit:0 rump.ifconfig shmif1 create
	atf_check -s exit:0 rump.ifconfig shmif1 linkstr ./shdom2
	atf_check -s exit:0 rump.ifconfig shmif1 10.0.2.1/24
	atf_check -s exit:0 rump.ifconfig mpls0 create up
	atf_check -s exit:0 rump.sysctl -q -w net.mpls.accept=1
	atf_check -s exit:0 rump.sysctl -q -w net.mpls.forwarding=1
	create_routes 10.0.2.2
	atf_check -s exit:0 rump.route -q add 10.0.5.0/24 10.0.2.2
	atf_check -s exit:0 /usr/sbin/ldpd ${LDP_FLAGS}

	# Setup third server
	export RUMP_SERVER=${RUMP_SERVER3}
	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig shmif0 linkstr ./shdom2
	atf_check -s exit:0 rump.ifconfig shmif0 10.0.2.2/24
	atf_check -s exit:0 rump.ifconfig shmif1 create
	atf_check -s exit:0 rump.ifconfig shmif1 linkstr ./shdom3
	atf_check -s exit:0 rump.ifconfig shmif1 10.0.3.1/24
	atf_check -s exit:0 rump.ifconfig mpls0 create up
	atf_check -s exit:0 rump.sysctl -q -w net.mpls.accept=1
	atf_check -s exit:0 rump.sysctl -q -w net.mpls.forwarding=1
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.forwarding=0
	atf_check -s exit:0 rump.route -q add 10.0.1.0/24 10.0.2.1
	atf_check -s exit:0 rump.route -q add 10.0.5.0/24 10.0.3.2
	create_routes 10.0.3.2
	atf_check -s exit:0 /usr/sbin/ldpd ${LDP_FLAGS}

	# Setup fourth server
	export RUMP_SERVER=${RUMP_SERVER4}
	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig shmif0 linkstr ./shdom3
	atf_check -s exit:0 rump.ifconfig shmif0 10.0.3.2/24
	atf_check -s exit:0 rump.ifconfig shmif1 create
	atf_check -s exit:0 rump.ifconfig shmif1 linkstr ./shdom4
	atf_check -s exit:0 rump.ifconfig shmif1 10.0.4.1/24
	atf_check -s exit:0 rump.sysctl -q -w net.mpls.accept=1
	atf_check -s exit:0 rump.sysctl -q -w net.mpls.forwarding=1
	atf_check -s exit:0 rump.ifconfig mpls0 create up
	atf_check -s exit:0 rump.route -q add 10.0.1.0/24 10.0.3.1
	atf_check -s exit:0 rump.route -q add 10.0.5.0/24 10.0.4.2
	create_routes 10.0.4.2
	atf_check -s exit:0 /usr/sbin/ldpd ${LDP_FLAGS}

	# Setup fifth server
	export RUMP_SERVER=${RUMP_SERVER5}
	atf_check -s exit:0 rump.ifconfig shmif0 create
	atf_check -s exit:0 rump.ifconfig shmif0 linkstr ./shdom4
	atf_check -s exit:0 rump.ifconfig shmif0 10.0.4.2/24
	atf_check -s exit:0 rump.ifconfig shmif1 create
	atf_check -s exit:0 rump.ifconfig shmif1 linkstr ./shdom5
	atf_check -s exit:0 rump.ifconfig shmif1 10.0.5.1/24
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.forwarding=0
	atf_check -s exit:0 rump.ifconfig mpls0 create up
	atf_check -s exit:0 rump.route -q add 10.0.1.0/24 10.0.4.1
	atf_check -s exit:0 /usr/sbin/ldpd ${LDP_FLAGS}

	unset RUMP_SERVER
	unset LD_PRELOAD
	unset RUMPHIJACK
}

wait_ldp_ok() {

	RUMP_SERVER=${RUMP_SERVER1} atf_check -s exit:0 -o ignore -e ignore \
		rump.ping -o -w 60 10.0.5.1
}

random_test() {

	RND=`od -A n -N 1 < /dev/urandom`
	RANDOM_B=`expr $RND % $MAX_B + 1`
	RND=`od -A n -N 1 < /dev/urandom`
	RANDOM_C=`expr $RND % $MAX_C`
	ping_one 10.${RANDOM_B}.${RANDOM_C}.1
}

docleanup() {

	RUMP_SERVER=${RUMP_SERVER1} rump.halt
	RUMP_SERVER=${RUMP_SERVER2} rump.halt
	RUMP_SERVER=${RUMP_SERVER3} rump.halt
	RUMP_SERVER=${RUMP_SERVER4} rump.halt
	RUMP_SERVER=${RUMP_SERVER5} rump.halt
}

ldp_static_body() {

	create_servers
	wait_ldp_ok

	for i in `seq 1 19`;
		do random_test
	done
}

ldp_static_cleanup() {

	docleanup
}

atf_init_test_cases() {

	atf_add_test_case ldp_static
}
