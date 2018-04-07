#	$NetBSD: net_common.sh,v 1.26.2.1 2018/04/07 04:12:20 pgoyette Exp $
#
# Copyright (c) 2016 Internet Initiative Japan Inc.
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

#
# Common utility functions for tests/net
#

HIJACKING="env LD_PRELOAD=/usr/lib/librumphijack.so \
    RUMPHIJACK=path=/rump,socket=all:nolocal,sysctl=yes"
ONEDAYISH="(23h5[0-9]m|1d0h0m)[0-9]+s ?"

extract_new_packets()
{
	local bus=$1
	local old=./.__old

	if [ ! -f $old ]; then
		old=/dev/null
	fi

	shmif_dumpbus -p - $bus 2>/dev/null |
	    tcpdump -n -e -r - 2>/dev/null > ./.__new
	diff -u $old ./.__new | grep '^+' | cut -d '+' -f 2   > ./.__diff
	mv -f ./.__new ./.__old
	cat ./.__diff
}

check_route()
{
	local target=$1
	local gw=$2
	local flags=${3:-\.\+}
	local ifname=${4:-\.\+}

	target=$(echo $target | sed 's/\./\\./g')
	if [ "$gw" = "" ]; then
		gw=".+"
	else
		gw=$(echo $gw | sed 's/\./\\./g')
	fi

	atf_check -s exit:0 -e ignore \
	    -o match:"^$target +$gw +$flags +- +- +.+ +$ifname" \
	    rump.netstat -rn
}

check_route_flags()
{

	check_route "$1" "" "$2" ""
}

check_route_gw()
{

	check_route "$1" "$2" "" ""
}

check_route_no_entry()
{
	local target=$(echo "$1" | sed 's/\./\\./g')

	atf_check -s exit:0 -e ignore -o not-match:"^$target" rump.netstat -rn
}

get_linklocal_addr()
{

	RUMP_SERVER=${1} rump.ifconfig ${2} inet6 |
	    awk "/fe80/ {sub(/%$2/, \"\"); sub(/\\/[0-9]*/, \"\"); print \$2;}"

	return 0
}

get_macaddr()
{

	RUMP_SERVER=${1} rump.ifconfig ${2} | awk '/address/ {print $2;}'
}

HTTPD_PID=./.__httpd.pid
start_httpd()
{
	local sock=$1
	local ip=$2
	local backup=$RUMP_SERVER

	export RUMP_SERVER=$sock

	# start httpd in daemon mode
	atf_check -s exit:0 env LD_PRELOAD=/usr/lib/librumphijack.so \
	    /usr/libexec/httpd -P $HTTPD_PID -i $ip -b -s $(pwd)

	export RUMP_SERVER=$backup

	sleep 3
}

stop_httpd()
{

	if [ -f $HTTPD_PID ]; then
		kill -9 $(cat $HTTPD_PID)
		rm -f $HTTPD_PID
		sleep 1
	fi
}

NC_PID=./.__nc.pid
start_nc_server()
{
	local sock=$1
	local port=$2
	local outfile=$3
	local proto=${4:-ipv4}
	local backup=$RUMP_SERVER
	local opts=

	export RUMP_SERVER=$sock

	if [ $proto = ipv4 ]; then
		opts="-l -4"
	else
		opts="-l -6"
	fi

	env LD_PRELOAD=/usr/lib/librumphijack.so nc $opts $port > $outfile &
	echo $! > $NC_PID

	if [ $proto = ipv4 ]; then
		$DEBUG && rump.netstat -a -f inet
	else
		$DEBUG && rump.netstat -a -f inet6
	fi

	export RUMP_SERVER=$backup

	sleep 1
}

stop_nc_server()
{

	if [ -f $NC_PID ]; then
		kill -9 $(cat $NC_PID)
		rm -f $NC_PID
		sleep 1
	fi
}

BASIC_LIBS="-lrumpnet -lrumpnet_net -lrumpnet_netinet -lrumpnet_shmif -lrumpdev"
FS_LIBS="$BASIC_LIBS -lrumpvfs -lrumpfs_ffs"
CRYPTO_LIBS="$BASIC_LIBS -lrumpvfs -lrumpdev_opencrypto \
    -lrumpkern_z -lrumpkern_crypto"
NPF_LIBS="$BASIC_LIBS -lrumpvfs -lrumpdev_bpf -lrumpnet_npf"

# We cannot keep variables between test phases, so need to store in files
_rump_server_socks=./.__socks
_rump_server_ifaces=./.__ifaces
_rump_server_buses=./.__buses

DEBUG_SYSCTL_ENTRIES="net.inet.arp.debug net.inet6.icmp6.nd6_debug \
    net.inet.ipsec.debug"

IPSEC_KEY_DEBUG=${IPSEC_KEY_DEBUG:-false}

_rump_server_start_common()
{
	local sock=$1
	local backup=$RUMP_SERVER

	shift 1

	atf_check -s exit:0 rump_server "$@" "$sock"

	if $DEBUG; then
		# Enable debugging features in the kernel
		export RUMP_SERVER=$sock
		for ent in $DEBUG_SYSCTL_ENTRIES; do
			if rump.sysctl -q $ent; then
				atf_check -s exit:0 rump.sysctl -q -w $ent=1
			fi
		done
		export RUMP_SERVER=$backup
	fi
	if $IPSEC_KEY_DEBUG; then
		# Enable debugging features in the kernel
		export RUMP_SERVER=$sock
		if rump.sysctl -q net.key.debug; then
			atf_check -s exit:0 \
			    rump.sysctl -q -w net.key.debug=0xffff
		fi
		export RUMP_SERVER=$backup
	fi

	echo $sock >> $_rump_server_socks
	$DEBUG && cat $_rump_server_socks
}

rump_server_start()
{
	local sock=$1
	local lib=
	local libs="$BASIC_LIBS"

	shift 1

	for lib
	do
		libs="$libs -lrumpnet_$lib"
	done

	_rump_server_start_common $sock $libs

	return 0
}

rump_server_fs_start()
{
	local sock=$1
	local lib=
	local libs="$FS_LIBS"

	shift 1

	for lib
	do
		libs="$libs -lrumpnet_$lib"
	done

	_rump_server_start_common $sock $libs

	return 0
}

rump_server_crypto_start()
{
	local sock=$1
	local lib=
	local libs="$CRYPTO_LIBS"

	shift 1

	for lib
	do
		libs="$libs -lrumpnet_$lib"
	done

	_rump_server_start_common $sock $libs

	return 0
}

rump_server_npf_start()
{
	local sock=$1
	local lib=
	local libs="$NPF_LIBS"

	shift 1

	for lib
	do
		libs="$libs -lrumpnet_$lib"
	done

	_rump_server_start_common $sock $libs

	return 0
}

rump_server_add_iface()
{
	local sock=$1
	local ifname=$2
	local bus=$3
	local backup=$RUMP_SERVER

	export RUMP_SERVER=$sock
	atf_check -s exit:0 rump.ifconfig $ifname create
	atf_check -s exit:0 rump.ifconfig $ifname linkstr $bus
	export RUMP_SERVER=$backup

	echo $sock $ifname >> $_rump_server_ifaces
	$DEBUG && cat $_rump_server_ifaces

	echo $bus >> $_rump_server_buses
	cat $_rump_server_buses |sort -u >./.__tmp
	mv -f ./.__tmp $_rump_server_buses
	$DEBUG && cat $_rump_server_buses

	return 0
}

rump_server_destroy_ifaces()
{
	local backup=$RUMP_SERVER
	local outout=ignore

	$DEBUG && cat $_rump_server_ifaces

	# Try to dump states before destroying interfaces
	for sock in $(cat $_rump_server_socks); do
		export RUMP_SERVER=$sock
		if $DEBUG; then
			output=save:/dev/stdout
		fi
		atf_check -s exit:0 -o $output rump.ifconfig
		atf_check -s exit:0 -o $output rump.netstat -nr
		# XXX still need hijacking
		atf_check -s exit:0 -o $output $HIJACKING rump.netstat -nai
		atf_check -s exit:0 -o $output rump.arp -na
		atf_check -s exit:0 -o $output rump.ndp -na
		atf_check -s exit:0 -o $output $HIJACKING ifmcstat
	done

	# XXX using pipe doesn't work. See PR bin/51667
	#cat $_rump_server_ifaces | while read sock ifname; do
	while read sock ifname; do
		export RUMP_SERVER=$sock
		if rump.ifconfig -l |grep -q $ifname; then
			atf_check -s exit:0 rump.ifconfig $ifname destroy
		fi
		atf_check -s exit:0 -o ignore rump.ifconfig
	done < $_rump_server_ifaces
	export RUMP_SERVER=$backup

	return 0
}

rump_server_halt_servers()
{
	local backup=$RUMP_SERVER

	$DEBUG && cat $_rump_server_socks
	for sock in $(cat $_rump_server_socks); do
		env RUMP_SERVER=$sock rump.halt
	done
	export RUMP_SERVER=$backup

	return 0
}

rump_server_dump_servers()
{
	local backup=$RUMP_SERVER

	$DEBUG && cat $_rump_server_socks
	for sock in $(cat $_rump_server_socks); do
		echo "### Dumping $sock"
		export RUMP_SERVER=$sock
		rump.ifconfig -av
		rump.netstat -nr
		# XXX still need hijacking
		$HIJACKING rump.netstat -nai
		rump.arp -na
		rump.ndp -na
		$HIJACKING ifmcstat
		$HIJACKING dmesg
	done
	export RUMP_SERVER=$backup

	if [ -f rump_server.core ]; then
		gdb -ex bt /usr/bin/rump_server rump_server.core
		strings rump_server.core |grep panic
	fi
	return 0
}

rump_server_dump_buses()
{

	if [ ! -f $_rump_server_buses ]; then
		return 0
	fi

	$DEBUG && cat $_rump_server_buses
	for bus in $(cat $_rump_server_buses); do
		echo "### Dumping $bus"
		shmif_dumpbus -p - $bus 2>/dev/null| tcpdump -n -e -r -
	done
	return 0
}

cleanup()
{

	rump_server_halt_servers
}

dump()
{

	rump_server_dump_servers
	rump_server_dump_buses
}

skip_if_qemu()
{
	if sysctl machdep.cpu_brand 2>/dev/null | grep QEMU >/dev/null 2>&1
	then
	    atf_skip "unreliable under qemu, skip until PR kern/43997 fixed"
	fi
}

test_create_destroy_common()
{
	local sock=$1
	local ifname=$2
	local test_address=${3:-false}
	local ipv4="10.0.0.1/24"
	local ipv6="fc00::1"

	export RUMP_SERVER=$sock

	atf_check -s exit:0 rump.ifconfig $ifname create
	atf_check -s exit:0 rump.ifconfig $ifname destroy

	atf_check -s exit:0 rump.ifconfig $ifname create
	atf_check -s exit:0 rump.ifconfig $ifname up
	atf_check -s exit:0 rump.ifconfig $ifname down
	atf_check -s exit:0 rump.ifconfig $ifname destroy

	# Destroy while UP
	atf_check -s exit:0 rump.ifconfig $ifname create
	atf_check -s exit:0 rump.ifconfig $ifname up
	atf_check -s exit:0 rump.ifconfig $ifname destroy

	if ! $test_address; then
		return
	fi

	# With an IPv4 address
	atf_check -s exit:0 rump.ifconfig $ifname create
	atf_check -s exit:0 rump.ifconfig $ifname inet $ipv4
	atf_check -s exit:0 rump.ifconfig $ifname up
	atf_check -s exit:0 rump.ifconfig $ifname destroy

	# With an IPv6 address
	atf_check -s exit:0 rump.ifconfig $ifname create
	atf_check -s exit:0 rump.ifconfig $ifname inet6 $ipv6
	atf_check -s exit:0 rump.ifconfig $ifname up
	atf_check -s exit:0 rump.ifconfig $ifname destroy

	unset RUMP_SERVER
}
