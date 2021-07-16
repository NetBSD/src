#	$NetBSD: t_cbq.sh,v 1.3 2021/07/16 02:33:32 ozaki-r Exp $
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

SOCK_LOCAL=unix://altq_local
SOCK_REMOTE=unix://altq_remote
BUS=bus_altq
TIMEOUT=3

# rumphijack can't handle AF_LOCAL socket (/var/run/altq_quip) correctly,
# so use the socket via the host.
HIJACKING_ALTQ="$HIJACKING,blanket=/dev/altq/altq:/dev/altq/cbq:/etc/altq.conf:/var/run/altqd.pid"

DEBUG=${DEBUG:-false}

IP_LOCAL1=10.0.0.1
IP_LOCAL2=10.0.1.1
IP_REMOTE11=10.0.0.2
IP_REMOTE12=10.0.0.22
IP_REMOTE13=10.0.0.23
IP_REMOTE21=10.0.1.2
IP_REMOTE22=10.0.1.22
ALTQD_PIDFILE=./pid

start_altqd()
{

	$HIJACKING_ALTQ altqd

	sleep 0.1
	if $HIJACKING_ALTQ test ! -f /var/run/altqd.pid; then
		sleep 1
	fi

	$HIJACKING_ALTQ test -f /var/run/altqd.pid
	if [ $? != 0 ]; then
		atf_fail "starting altqd failed"
	fi

	$HIJACKING_ALTQ cat /var/run/altqd.pid > $ALTQD_PIDFILE
}

start_altqd_basic()
{

	export RUMP_SERVER=$SOCK_LOCAL

	$HIJACKING_ALTQ mkdir -p /rump/etc
	$HIJACKING_ALTQ mkdir -p /rump/var/run

	cat > ./altq.conf <<-EOF
	interface shmif0 cbq
	class cbq shmif0 root_class NULL pbandwidth 100
	class cbq shmif0 normal_class root_class pbandwidth 50 default
	    filter shmif0 normal_class $IP_REMOTE11 0 0 0 0
	class cbq shmif0 drop_class root_class pbandwidth 0
	    filter shmif0 drop_class $IP_REMOTE12 0 0 0 0
	EOF
	$DEBUG && cat ./altq.conf
	atf_check -s exit:0 $HIJACKING_ALTQ cp ./altq.conf /rump/etc/altq.conf
	atf_check -s exit:0 $HIJACKING_ALTQ test -f /rump/etc/altq.conf

	start_altqd

	$DEBUG && $HIJACKING_ALTQ altqstat -s
	$HIJACKING_ALTQ altqstat -c 1 >./out
	$DEBUG && cat ./out
	atf_check -s exit:0 \
	    -o match:"altqstat: cbq on interface shmif0" \
	    -o match:'Class 1 on Interface shmif0: root_class' \
	    -o match:'Class 2 on Interface shmif0: normal_class' \
	    -o match:'Class 3 on Interface shmif0: ctl_class' \
	    -o match:'Class 4 on Interface shmif0: drop_class' \
	    cat ./out
	rm -f ./out
}

shutdown_altqd()
{
	local pid="$(cat $ALTQD_PIDFILE)"

	if [ -n "$pid" ]; then
		pgrep -x altqd | grep -q $pid
		if [ $? = 0 ]; then
			kill $(cat $ALTQD_PIDFILE)
			sleep 1
		fi
		$DEBUG && pgrep -x altqd
	fi
}

check_counter()
{
	local file=$1
	local name=$2
	local match="$3"

	grep -A 8 ${name}_class $file > $file.$name
	atf_check -s exit:0 -o match:"$match" cat $file.$name
	rm -f $file.$name
}

test_altq_cbq_basic_ipv4()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore rump.ping"
	local opts="-q -c 1 -w 1"

	rump_server_fs_start $SOCK_LOCAL local altq
	rump_server_start $SOCK_REMOTE

	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	$ifconfig shmif0 inet $IP_LOCAL1/24
	export RUMP_SERVER=$SOCK_REMOTE
	$ifconfig shmif0 inet $IP_REMOTE11/24
	$ifconfig shmif0 inet $IP_REMOTE12/24 alias
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_LOCAL
	# Invoke ARP
	$ping $opts $IP_REMOTE11
	$ping $opts $IP_REMOTE12

	start_altqd_basic

	export RUMP_SERVER=$SOCK_LOCAL
	$ping $opts $IP_REMOTE11

	$HIJACKING_ALTQ altqstat -c 1 >./out
	$DEBUG && cat ./out

	check_counter ./out normal 'pkts: 1'
	check_counter ./out root   'pkts: 1'
	check_counter ./out drop   'pkts: 0'

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s not-exit:0 -o ignore -e match:"No buffer space available" \
	    rump.ping $opts $IP_REMOTE12

	$HIJACKING_ALTQ altqstat -c 1 >./out
	$DEBUG && cat ./out

	check_counter ./out drop   'drops: 1'
	check_counter ./out drop   'pkts: 0'
	check_counter ./out normal 'pkts: 1'
	check_counter ./out root   'pkts: 1'

	rm -f ./out

	shutdown_altqd

	rump_server_destroy_ifaces
}

start_altqd_multi_ifaces()
{

	export RUMP_SERVER=$SOCK_LOCAL

	$HIJACKING_ALTQ mkdir -p /rump/etc
	$HIJACKING_ALTQ mkdir -p /rump/var/run

	cat > ./altq.conf <<-EOF
	interface shmif0 cbq
	class cbq shmif0 root_class NULL pbandwidth 100
	class cbq shmif0 normal_class root_class pbandwidth 50 default
	    filter shmif0 normal_class $IP_REMOTE11 0 0 0 0
	class cbq shmif0 drop_class root_class pbandwidth 0
	    filter shmif0 drop_class $IP_REMOTE12 0 0 0 0
	interface shmif1 cbq
	class cbq shmif1 root_class NULL pbandwidth 100
	class cbq shmif1 normal_class root_class pbandwidth 50 default
	    filter shmif1 normal_class $IP_REMOTE21 0 0 0 0
	class cbq shmif1 drop_class root_class pbandwidth 0
	    filter shmif1 drop_class $IP_REMOTE22 0 0 0 0
	EOF
	$DEBUG && cat ./altq.conf
	atf_check -s exit:0 $HIJACKING_ALTQ cp ./altq.conf /rump/etc/altq.conf
	$HIJACKING_ALTQ test -f /rump/etc/altq.conf

	start_altqd

	$DEBUG && $HIJACKING_ALTQ altqstat -s

	$HIJACKING_ALTQ altqstat -c 1 -i shmif0 >./out
	$DEBUG && cat ./out
	atf_check -s exit:0 \
	    -o match:"altqstat: cbq on interface shmif0" \
	    -o match:'Class 1 on Interface shmif0: root_class' \
	    -o match:'Class 2 on Interface shmif0: normal_class' \
	    -o match:'Class 3 on Interface shmif0: ctl_class' \
	    -o match:'Class 4 on Interface shmif0: drop_class' \
	    cat ./out

	$HIJACKING_ALTQ altqstat -c 1 -i shmif1 >./out
	$DEBUG && cat ./out
	atf_check -s exit:0 \
	    -o match:"altqstat: cbq on interface shmif1" \
	    -o match:'Class 1 on Interface shmif1: root_class' \
	    -o match:'Class 2 on Interface shmif1: normal_class' \
	    -o match:'Class 3 on Interface shmif1: ctl_class' \
	    -o match:'Class 4 on Interface shmif1: drop_class' \
	    cat ./out

	rm -f ./out
}

test_altq_cbq_multi_ifaces_ipv4()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore rump.ping"
	local opts="-q -c 1 -w 1"

	rump_server_fs_start $SOCK_LOCAL local altq
	rump_server_start $SOCK_REMOTE

	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_LOCAL shmif1 $BUS
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	$ifconfig shmif0 inet $IP_LOCAL1/24
	$ifconfig shmif1 inet $IP_LOCAL2/24
	export RUMP_SERVER=$SOCK_REMOTE
	$ifconfig shmif0 inet $IP_REMOTE11/24
	$ifconfig shmif0 inet $IP_REMOTE12/24 alias
	$ifconfig shmif0 inet $IP_REMOTE21/24 alias
	$ifconfig shmif0 inet $IP_REMOTE22/24 alias
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_LOCAL
	# Invoke ARP
	$ping $opts $IP_REMOTE11
	$ping $opts $IP_REMOTE12
	$ping $opts $IP_REMOTE21
	$ping $opts $IP_REMOTE22

	start_altqd_multi_ifaces

	export RUMP_SERVER=$SOCK_LOCAL
	$ping $opts $IP_REMOTE11

	$HIJACKING_ALTQ altqstat -c 1 -i shmif0 >./out
	$DEBUG && cat ./out

	check_counter ./out normal 'pkts: 1'
	check_counter ./out root   'pkts: 1'
	check_counter ./out drop   'pkts: 0'

	$ping $opts $IP_REMOTE21

	$HIJACKING_ALTQ altqstat -c 1 -i shmif1 >./out
	$DEBUG && cat ./out

	check_counter ./out normal 'pkts: 1'
	check_counter ./out root   'pkts: 1'
	check_counter ./out drop   'pkts: 0'

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s not-exit:0 -o ignore -e match:"No buffer space available" \
	    rump.ping $opts $IP_REMOTE12

	$HIJACKING_ALTQ altqstat -c 1 -i shmif0 >./out
	$DEBUG && cat ./out

	check_counter ./out drop   'drops: 1'
	check_counter ./out drop   'pkts: 0'
	check_counter ./out normal 'pkts: 1'
	check_counter ./out root   'pkts: 1'

	atf_check -s not-exit:0 -o ignore -e match:"No buffer space available" \
	    rump.ping $opts $IP_REMOTE22

	$HIJACKING_ALTQ altqstat -c 1 -i shmif1 >./out
	$DEBUG && cat ./out

	check_counter ./out drop   'drops: 1'
	check_counter ./out drop   'pkts: 0'
	check_counter ./out normal 'pkts: 1'
	check_counter ./out root   'pkts: 1'

	rm -f ./out

	shutdown_altqd

	rump_server_destroy_ifaces
}

start_altqd_options()
{

	export RUMP_SERVER=$SOCK_LOCAL

	$HIJACKING_ALTQ mkdir -p /rump/etc
	$HIJACKING_ALTQ mkdir -p /rump/var/run

	# - no-tbr and no-control are specified
	# - root_class is the default class
	cat > ./altq.conf <<-EOF
	interface shmif0 cbq no-tbr no-control
	class cbq shmif0 root_class NULL pbandwidth 100 default
	class cbq shmif0 normal_class root_class pbandwidth 50
	    filter shmif0 normal_class $IP_REMOTE11 0 0 0 0
	class cbq shmif0 drop_class root_class pbandwidth 0
	    filter shmif0 drop_class $IP_REMOTE12 0 0 0 0
	EOF
	$DEBUG && cat ./altq.conf
	atf_check -s exit:0 $HIJACKING_ALTQ cp ./altq.conf /rump/etc/altq.conf
	$HIJACKING_ALTQ test -f /rump/etc/altq.conf

	start_altqd

	$DEBUG && $HIJACKING_ALTQ altqstat -s
	$HIJACKING_ALTQ altqstat -c 1 >./out
	$DEBUG && cat ./out
	atf_check -s exit:0 \
	    -o match:"altqstat: cbq on interface shmif0" \
	    -o match:'Class 1 on Interface shmif0: root_class' \
	    -o match:'Class 2 on Interface shmif0: normal_class' \
	    -o match:'Class 3 on Interface shmif0: drop_class' \
	    cat ./out
	atf_check -s exit:0 -o not-match:'shmif0: ctl_class' cat ./out

	rm -f ./out
}

test_altq_cbq_options_ipv4()
{
	local ifconfig="atf_check -s exit:0 rump.ifconfig"
	local ping="atf_check -s exit:0 -o ignore rump.ping"
	local opts="-q -c 1 -w 1"

	rump_server_fs_start $SOCK_LOCAL local altq
	rump_server_start $SOCK_REMOTE

	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_REMOTE shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	$ifconfig shmif0 inet $IP_LOCAL1/24
	export RUMP_SERVER=$SOCK_REMOTE
	$ifconfig shmif0 inet $IP_REMOTE11/24
	$ifconfig shmif0 inet $IP_REMOTE12/24 alias
	$ifconfig shmif0 inet $IP_REMOTE13/24 alias
	$ifconfig -w 10

	export RUMP_SERVER=$SOCK_LOCAL
	# Invoke ARP
	$ping $opts $IP_REMOTE11
	$ping $opts $IP_REMOTE12
	$ping $opts $IP_REMOTE13

	start_altqd_options

	export RUMP_SERVER=$SOCK_LOCAL
	$ping $opts $IP_REMOTE11

	$HIJACKING_ALTQ altqstat -c 1 >./out
	$DEBUG && cat ./out

	check_counter ./out normal 'pkts: 1'
	check_counter ./out root   'pkts: 1'
	check_counter ./out drop   'pkts: 0'

	atf_check -s not-exit:0 -o ignore -e match:"No buffer space available" \
	    rump.ping $opts $IP_REMOTE12

	$HIJACKING_ALTQ altqstat -c 1 >./out
	$DEBUG && cat ./out

	check_counter ./out drop   'drops: 1'
	check_counter ./out drop   'pkts: 0'
	check_counter ./out normal 'pkts: 1'
	check_counter ./out root   'pkts: 1'

	# The packet goes to the default class
	$ping $opts $IP_REMOTE13

	$HIJACKING_ALTQ altqstat -c 1 >./out
	$DEBUG && cat ./out

	check_counter ./out drop   'pkts: 0'
	check_counter ./out normal 'pkts: 1'
	check_counter ./out root   'pkts: 2'

	rm -f ./out

	shutdown_altqd

	rump_server_destroy_ifaces
}

add_test_case()
{
	local algo=$1
	local type=$2
	local ipproto=$3

	name="altq_${algo}_${type}_${ipproto}"
	desc="Tests for ALTQ $algo (${type}) on ${ipproto}"

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server altqd altqstat
	    }
	    ${name}_body() {
	        test_altq_${algo}_${type}_${ipproto}
	    }
	    ${name}_cleanup() {
	        shutdown_altqd
	        \$DEBUG && dump
	        cleanup
	    }
	"
	atf_add_test_case ${name}
}

atf_init_test_cases()
{

	add_test_case cbq basic        ipv4
	add_test_case cbq multi_ifaces ipv4
	add_test_case cbq options      ipv4
}
