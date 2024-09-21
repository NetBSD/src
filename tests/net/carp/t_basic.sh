#	$NetBSD: t_basic.sh,v 1.8.8.1 2024/09/21 12:26:48 martin Exp $
#
# Copyright (c) 2017 Internet Initiative Japan Inc.
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

SOCK_CLIENT=unix://carp_client
SOCK_MASTER=unix://carp_master
SOCK_BACKUP=unix://carp_backup
BUS=bus_carp
TIMEOUT=3

DEBUG=${DEBUG:-false}

IP_CLIENT=10.1.1.240
IP_MASTER=10.1.1.1
IP_BACKUP=10.1.1.2
IP_CARP=10.1.1.100

setup_carp()
{
	local sock=$1
	local master=$2
	local carpdevip=$3
	local carpif= ip= advskew=

	if $master; then
		carpif=carp0
		ip=$IP_MASTER
		advskew=0
	else
		carpif=carp1
		ip=$IP_BACKUP
		advskew=200
	fi

	export RUMP_SERVER=$sock
	if $DEBUG; then
		atf_check -s exit:0 -o match:'0.->.1' \
		    rump.sysctl -w net.inet.carp.log=1
	fi
	rump_server_add_iface $sock $carpif
	if [ $carpdevip = yes ]; then
		atf_check -s exit:0 rump.ifconfig shmif0 $ip/24 up
		atf_check -s exit:0 rump.ifconfig $carpif \
		    vhid 175 advskew $advskew advbase 1 pass s3cret \
		    $IP_CARP netmask 255.255.255.0
	else
		atf_check -s exit:0 rump.ifconfig shmif0 up
		atf_check -s exit:0 rump.ifconfig $carpif \
		    vhid 175 advskew $advskew advbase 1 pass s3cret \
		    carpdev shmif0 $IP_CARP netmask 255.255.255.0
	fi
	atf_check -s exit:0 rump.ifconfig -w 10
}

wait_handover()
{
	local i=0

	export RUMP_SERVER=$SOCK_CLIENT

	while [ $i -ne 5 ]; do
		$DEBUG && echo "Trying ping $IP_CARP"
		rump.ping -n -w 1 -c 1 $IP_CARP >/dev/null
		if [ $? = 0 ]; then
			$DEBUG && echo "Passed ping $IP_CARP"
			break;
		fi
		$DEBUG && echo "Failed ping $IP_CARP"
		i=$((i + 1))
	done

	if [ $i -eq 5 ]; then
		atf_fail "Failed to failover (5 sec)"
	fi
}

test_carp_handover_ipv4()
{
	local op=$1
	local carpdevip=$2

	rump_server_start $SOCK_CLIENT
	rump_server_start $SOCK_MASTER
	rump_server_start $SOCK_BACKUP

	rump_server_add_iface $SOCK_CLIENT shmif0 $BUS
	rump_server_add_iface $SOCK_MASTER shmif0 $BUS
	rump_server_add_iface $SOCK_BACKUP shmif0 $BUS

	setup_carp $SOCK_MASTER true $carpdevip
	setup_carp $SOCK_BACKUP false $carpdevip

	export RUMP_SERVER=$SOCK_CLIENT
	atf_check -s exit:0 rump.ifconfig shmif0 $IP_CLIENT/24 up
	atf_check -s exit:0 rump.ifconfig -w 10

	if [ $carpdevip = yes ]; then
		# Check that the primary addresses are up
		atf_check -s exit:0 -o ignore \
		    rump.ping -n -w $TIMEOUT -c 1 $IP_MASTER
		atf_check -s exit:0 -o ignore \
		    rump.ping -n -w $TIMEOUT -c 1 $IP_BACKUP
	fi

	# Give carp a while to croak
	sleep 4

	# Check state
	export RUMP_SERVER=$SOCK_MASTER
	$DEBUG && rump.ifconfig
	atf_check -s exit:0 -o match:'carp: MASTER carpdev shmif0' \
	    rump.ifconfig carp0
	export RUMP_SERVER=$SOCK_BACKUP
	$DEBUG && rump.ifconfig
	atf_check -s exit:0 -o match:'carp: BACKUP carpdev shmif0' \
	    rump.ifconfig carp1
	export RUMP_SERVER=$SOCK_CLIENT

	# Check that the shared IP works
	atf_check -s exit:0 -o ignore \
	    rump.ping -n -w $TIMEOUT -c 1 $IP_CARP

	# KILLING SPREE
	if [ $op = halt ]; then
		env RUMP_SERVER=$SOCK_MASTER rump.halt
	elif [ $op = ifdown ]; then
		env RUMP_SERVER=$SOCK_MASTER rump.ifconfig shmif0 down
	fi
	sleep 1

	# Check that primary is now dead
	if [ $carpdevip = yes ]; then
		atf_check -s not-exit:0 -o ignore \
		    rump.ping -n -w $TIMEOUT -c 1 $IP_MASTER
	else
		# XXX how to check?
	fi

	# Do it in installments. carp will cluck meanwhile
	wait_handover

	# Check state
	export RUMP_SERVER=$SOCK_BACKUP
	$DEBUG && rump.ifconfig
	atf_check -s exit:0 -o match:'carp: MASTER carpdev shmif0' \
	    rump.ifconfig carp1

	if [ $op = ifdown ]; then
		rump_server_destroy_ifaces
	fi
}

IP6_CLIENT=fd00:1::240
IP6_MASTER=fd00:1::1
IP6_BACKUP=fd00:1::2
IP6_CARP=fd00:1::100

setup_carp6()
{
	local sock=$1
	local master=$2
	local carpdevip=$3
	local carpif= ip= advskew=

	if $master; then
		carpif=carp0
		ip=$IP6_MASTER
		advskew=0
	else
		carpif=carp1
		ip=$IP6_BACKUP
		advskew=200
	fi

	export RUMP_SERVER=$sock
	if $DEBUG; then
		atf_check -s exit:0 -o match:'0.->.1' \
		    rump.sysctl -w net.inet.carp.log=1
	fi
	rump_server_add_iface $sock $carpif
	if [ $carpdevip = yes ]; then
		atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip up
		atf_check -s exit:0 rump.ifconfig $carpif inet6 \
		    vhid 175 advskew $advskew advbase 1 pass s3cret $IP6_CARP
	else
		atf_check -s exit:0 rump.ifconfig shmif0 up
		atf_check -s exit:0 rump.ifconfig $carpif inet6 \
		    vhid 175 advskew $advskew advbase 1 pass s3cret \
		    carpdev shmif0 $IP6_CARP
	fi
	atf_check -s exit:0 rump.ifconfig -w 10
}

wait_carp6_handover()
{
	local i=0

	export RUMP_SERVER=$SOCK_CLIENT

	while [ $i -ne 5 ]; do
		$DEBUG && echo "Trying ping6 $IP6_CARP"
		rump.ping6 -n -X 1 -c 1 $IP6_CARP >/dev/null
		if [ $? = 0 ]; then
			$DEBUG && echo "Passed ping $IP6_CARP"
			break;
		fi
		$DEBUG && echo "Failed ping6 $IP6_CARP"
		i=$((i + 1))
	done

	if [ $i -eq 5 ]; then
		atf_fail "Failed to failover (5 sec)"
	fi
}

test_carp_handover_ipv6()
{
	local op=$1
	local carpdevip=$2

	rump_server_start $SOCK_CLIENT netinet6
	rump_server_start $SOCK_MASTER netinet6
	rump_server_start $SOCK_BACKUP netinet6

	rump_server_add_iface $SOCK_CLIENT shmif0 $BUS
	rump_server_add_iface $SOCK_MASTER shmif0 $BUS
	rump_server_add_iface $SOCK_BACKUP shmif0 $BUS

	setup_carp6 $SOCK_MASTER true $carpdevip
	setup_carp6 $SOCK_BACKUP false $carpdevip

	export RUMP_SERVER=$SOCK_CLIENT
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $IP6_CLIENT up
	atf_check -s exit:0 rump.ifconfig -w 10

	if [ $carpdevip = yes ]; then
		# Check that the primary addresses are up
		atf_check -s exit:0 -o ignore \
		    rump.ping6 -n -X $TIMEOUT -c 1 $IP6_MASTER
		atf_check -s exit:0 -o ignore \
		    rump.ping6 -n -X $TIMEOUT -c 1 $IP6_BACKUP
	fi

	# Give carp a while to croak
	sleep 4

	# Check state
	export RUMP_SERVER=$SOCK_MASTER
	$DEBUG && rump.ifconfig
	atf_check -s exit:0 -o match:'carp: MASTER carpdev shmif0' \
	    rump.ifconfig carp0
	export RUMP_SERVER=$SOCK_BACKUP
	$DEBUG && rump.ifconfig
	atf_check -s exit:0 -o match:'carp: BACKUP carpdev shmif0' \
	    rump.ifconfig carp1
	export RUMP_SERVER=$SOCK_CLIENT

	# Check that the shared IP works
	atf_check -s exit:0 -o ignore \
	    rump.ping6 -n -X $TIMEOUT -c 1 $IP6_CARP

	# KILLING SPREE
	if [ $op = halt ]; then
		env RUMP_SERVER=$SOCK_MASTER rump.halt
	elif [ $op = ifdown ]; then
		env RUMP_SERVER=$SOCK_MASTER rump.ifconfig shmif0 down
	fi
	sleep 1

	# Check that primary is now dead
	if [ $carpdevip = yes ]; then
		atf_check -s not-exit:0 -o ignore \
		    rump.ping6 -n -X $TIMEOUT -c 1 $IP6_MASTER
	else
		# XXX how to check?
	fi

	# Do it in installments. carp will cluck meanwhile
	wait_carp6_handover

	# Check state
	export RUMP_SERVER=$SOCK_BACKUP
	$DEBUG && rump.ifconfig
	atf_check -s exit:0 -o match:'carp: MASTER carpdev shmif0' \
	    rump.ifconfig carp1

	if [ $op = ifdown ]; then
		rump_server_destroy_ifaces
	fi
}

add_test_case()
{
	local ipproto=$1
	local halt=$2
	local carpdevip=$3
	local expected_failure_code=

	name="carp_handover_${ipproto}_${halt}"
	desc="Tests for CARP (${ipproto}) handover on ${halt}"
	if [ $carpdevip = yes ]; then
		name="${name}_carpdevip"
		desc="$desc with carpdev IP"
	else
		name="${name}_nocarpdevip"
		desc="$desc without carpdev IP"
	fi

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server
	    }
	    ${name}_body() {
	        $expected_failure_code
	        test_carp_handover_${ipproto} $halt $carpdevip
	        if [ $halt != halt ]; then
	             rump_server_destroy_ifaces
	        fi
	    }
	    ${name}_cleanup() {
	        \$DEBUG && dump
	        cleanup
	    }
	"
	atf_add_test_case ${name}
}

atf_init_test_cases()
{
	local proto= halt= carpdevip=

	for proto in ipv4 ipv6; do
		for halt in halt ifdown; do
			for carpdevip in yes no; do
				add_test_case $proto $halt $carpdevip
			done
		done
	done
}
