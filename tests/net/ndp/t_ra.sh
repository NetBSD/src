#	$NetBSD: t_ra.sh,v 1.32 2017/11/25 07:58:47 kre Exp $
#
# Copyright (c) 2015 Internet Initiative Japan Inc.
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

RUMPSRV=unix://r1
RUMPSRV1_2=unix://r12
RUMPCLI=unix://r2
RUMPSRV3=unix://r3
RUMPSRV4=unix://r4
IP6SRV=fc00:1::1
IP6SRV1_2=fc00:1::2
IP6SRV_PREFIX=fc00:1:
IP6CLI=fc00:2::2
IP6SRV3=fc00:3::1
IP6SRV3_PREFIX=fc00:3:
IP6SRV4=fc00:4::1
IP6SRV4_PREFIX=fc00:4:
PIDFILE=./rump.rtadvd.pid
PIDFILE1_2=./rump.rtadvd.pid12
PIDFILE3=./rump.rtadvd.pid3
PIDFILE4=./rump.rtadvd.pid4
CONFIG=./rtadvd.conf
DEBUG=${DEBUG:-false}

RUMP_PROGS="rump_server rump.rtadvd rump.ndp rump.ifconfig rump.netstat"

init_server()
{

	export RUMP_SERVER=$1
	atf_check -s exit:0 -o match:'0.->.1' \
		rump.sysctl -w net.inet6.ip6.forwarding=1
	export LD_PRELOAD=/usr/lib/librumphijack.so
	atf_check -s exit:0 mkdir -p /rump/var/chroot/rtadvd
	unset LD_PRELOAD
	unset RUMP_SERVER
}

setup_shmif0()
{
	local sock=$1
	local IP6ADDR=$2

	rump_server_add_iface $sock shmif0 bus1

	export RUMP_SERVER=$sock
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 ${IP6ADDR}
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig -w 10

	$DEBUG && rump.ifconfig
}

kill_rtadvd()
{
	local pid=$(cat "$1")

	test -n "$pid" && {
		case "$pid" in
		*[!0-9]*)	return 1;;
		esac
		test "$pid" -gt 1 && kill -s KILL "${pid}"
	}
	rm -f "$1"
}

terminate_rtadvd()
{
	local pidfile=$1
	local n=5

	if ! [ -f "$pidfile" ]; then
		return
	fi

	local pid=$(cat "$pidfile")

	test -n "${pid}" && kill -s TERM "${pid}"

	# Note, rtadvd cannot remove its own pidfile, it chroots after
	# creating it (and if it chroot'd first, we would not be able to
	# control where it puts the thing, and so it would be useless.)
	# However, it does truncate it... so watch for that.
	while [ -s "$pidfile" ]; do
		n=$((n - 1))
		if [ "$n" = 0 ]; then
			kill_rtadvd "$pidfile"
			return
		fi
		sleep 0.2
	done
	# and finally complete the cleanup that rtadvd did not do.
	rm -f "$pidfile"
}

create_rtadvdconfig()
{
	cat << _EOF > ${CONFIG}
shmif0:\\
	:mtu#1300:maxinterval#4:mininterval#3:
_EOF
}

create_rtadvdconfig_rltime()
{
	cat << _EOF > ${CONFIG}
shmif0:\\
	:mtu#1300:maxinterval#4:mininterval#3:rltime#$1:
_EOF
	$DEBUG && cat ${CONFIG}
}

create_rtadvdconfig_vltime()
{
	cat << _EOF > ${CONFIG}
shmif0:\\
	:mtu#1300:maxinterval#4:mininterval#3:addr="$1":vltime#$2:
_EOF
	$DEBUG && cat ${CONFIG}
}

RA_count()
{
	RUMP_SERVER="$1" rump.netstat -p icmp6 | sed -n '
		$ {
			s/^.*$/0/p
			q
		}
		/router advertisement:/!d
		s/.*router advertisement: *//
		s/[ 	]*$//
		s/^$/0/
		p
		q
	'
}

await_RA()
{
	local N=$(RA_count "$1")
	while [ "$(RA_count "$1")" -le "$N" ]; do
		sleep 0.2
	done
}

start_rtadvd()
{
	local RUMP_SERVER="$1"
	export RUMP_SERVER

	atf_check -s exit:0 -e ignore \
	    rump.rtadvd -D -c ${CONFIG} -p "$2" shmif0

	# don't wait for the pid file to appear and get a pid in it.
	# we look for receiving RAs instead, must more reliable
	# extra delay here increases possibility of RA arriving before
	# we start looking (which means we then wait for the next .. boring!)
}

check_entries()
{
	local RUMP_SERVER="$1"
	local srv=$2
	local addr_prefix=$3
	local mac_srv= ll_srv=
	export RUMP_SERVER

	ll_srv=$(get_linklocal_addr "$srv" shmif0)
	mac_srv=$(get_macaddr "$srv" shmif0)

	$DEBUG && dump_entries
	atf_check -s exit:0 -o match:if=shmif0 rump.ndp -r
	atf_check -s exit:0 -o match:advertised  \
	    -o match:"${ll_srv}%shmif0 \(reachable\)" \
		rump.ndp -p
	atf_check -s exit:0 -o match:linkmtu=1300 rump.ndp -n -i shmif0
	atf_check -s exit:0 \
	    -o match:"$ll_srv%shmif0 +$mac_srv +shmif0 +$ONEDAYISH S R" \
	    -o not-match:"$addr_prefix"  \
		    rump.ndp -n -a
	atf_check -s exit:0 \
	    -o match:"$addr_prefix.+<(TENTATIVE,)?AUTOCONF>" \
	    rump.ifconfig shmif0 inet6
}

dump_entries()
{
	local marker="+-+-+-+-+-+-+-+-+"

	printf '%s %s\n' "$marker" 'ndp -n -a'
	rump.ndp -n -a
	printf '%s %s\n' "$marker" 'ndp -p'
	rump.ndp -p
	printf '%s %s\n' "$marker" 'ndp -r'
	rump.ndp -r
	printf '%s\n' "$marker"
}

atf_test_case ra_basic cleanup
ra_basic_head()
{

	atf_set "descr" "Tests for basic functions of router advaertisement(RA)"
	atf_set "require.progs" "${RUMP_PROGS}"
}

ra_basic_body()
{

	rump_server_fs_start $RUMPSRV netinet6
	rump_server_start $RUMPCLI netinet6

	setup_shmif0 ${RUMPSRV} ${IP6SRV}
	init_server $RUMPSRV

	setup_shmif0 ${RUMPCLI} ${IP6CLI}
	export RUMP_SERVER=${RUMPCLI}
	$DEBUG && rump.ndp -n -a
	atf_check -s exit:0 -o match:'= 0' \
		rump.sysctl net.inet6.ip6.accept_rtadv
	unset RUMP_SERVER

	create_rtadvdconfig
	start_rtadvd $RUMPSRV $PIDFILE
	await_RA "${RUMPCLI}"

	export RUMP_SERVER=${RUMPCLI}
	atf_check -s exit:0 -o empty rump.ndp -r
	atf_check -s exit:0 -o not-match:advertised rump.ndp -p
	atf_check -s exit:0 -o match:linkmtu=0 rump.ndp -n -i shmif0
	atf_check -s exit:0 -o not-match:'S R' -o not-match:fc00:1: \
		rump.ndp -n -a
	atf_check -s exit:0 -o not-match:fc00:1: rump.ifconfig shmif0 inet6
	unset RUMP_SERVER

	terminate_rtadvd $PIDFILE

	RUMP_SERVER=${RUMPCLI} atf_check -s exit:0 -o match:'0 -> 1' \
		rump.sysctl -w net.inet6.ip6.accept_rtadv=1

	start_rtadvd $RUMPSRV $PIDFILE
	await_RA "${RUMPCLI}"

	check_entries $RUMPCLI $RUMPSRV $IP6SRV_PREFIX

	terminate_rtadvd $PIDFILE

	rump_server_destroy_ifaces
}

ra_basic_cleanup()
{

	$DEBUG && dump
	terminate_rtadvd $PIDFILE
	cleanup
}

atf_test_case ra_flush_prefix_entries cleanup
ra_flush_prefix_entries_head()
{

	atf_set "descr" "Tests for flushing prefixes (ndp -P)"
	atf_set "require.progs" "${RUMP_PROGS}"
}

ra_flush_prefix_entries_body()
{

	rump_server_fs_start $RUMPSRV netinet6
	rump_server_start $RUMPCLI netinet6

	setup_shmif0 ${RUMPSRV} ${IP6SRV}
	setup_shmif0 ${RUMPCLI} ${IP6CLI}

	init_server $RUMPSRV

	create_rtadvdconfig

	export RUMP_SERVER=${RUMPCLI}
	atf_check -s exit:0 -o match:'0 -> 1' \
		rump.sysctl -w net.inet6.ip6.accept_rtadv=1
	unset RUMP_SERVER

	start_rtadvd $RUMPSRV $PIDFILE
	await_RA "${RUMPCLI}"

	check_entries $RUMPCLI $RUMPSRV $IP6SRV_PREFIX

	export RUMP_SERVER=${RUMPCLI}

	# Terminate rtadvd to prevent new RA messages from coming
	terminate_rtadvd $PIDFILE

	# Flush all the entries in the prefix list
	atf_check -s exit:0 rump.ndp -P

	$DEBUG && dump_entries
	atf_check -s exit:0 -o match:if=shmif0 rump.ndp -r
	atf_check -s exit:0 -o empty rump.ndp -p
	atf_check -s exit:0 -o match:linkmtu=1300 rump.ndp -n -i shmif0
	atf_check -s exit:0 -o match:"$ONEDAYISH S R" -o not-match:fc00:1: \
		rump.ndp -n -a
	atf_check -s exit:0 -o not-match:'fc00:1:' rump.ifconfig shmif0 inet6
	unset RUMP_SERVER

	rump_server_destroy_ifaces
}

ra_flush_prefix_entries_cleanup()
{

	$DEBUG && dump
	terminate_rtadvd $PIDFILE
	cleanup
}

atf_test_case ra_flush_defrouter_entries cleanup
ra_flush_defrouter_entries_head()
{

	atf_set "descr" "Tests for flushing default routers (ndp -R)"
	atf_set "require.progs" "${RUMP_PROGS}"
}

ra_flush_defrouter_entries_body()
{

	rump_server_fs_start $RUMPSRV netinet6
	rump_server_start $RUMPCLI netinet6

	setup_shmif0 ${RUMPSRV} ${IP6SRV}
	setup_shmif0 ${RUMPCLI} ${IP6CLI}

	init_server $RUMPSRV

	create_rtadvdconfig

	RUMP_SERVER=${RUMPCLI} atf_check -s exit:0 -o match:'0 -> 1' \
		rump.sysctl -w net.inet6.ip6.accept_rtadv=1

	start_rtadvd $RUMPSRV $PIDFILE
	await_RA "${RUMPCLI}"

	check_entries $RUMPCLI $RUMPSRV $IP6SRV_PREFIX

	export RUMP_SERVER=${RUMPCLI}

	# Terminate rtadvd to prevent new RA messages from coming
	terminate_rtadvd $PIDFILE

	# Flush all the entries in the default router list
	atf_check -s exit:0 rump.ndp -R

	$DEBUG && dump_entries
	atf_check -s exit:0 -o empty rump.ndp -r
	atf_check -s exit:0 -o match:'No advertising router' rump.ndp -p
	atf_check -s exit:0 -o match:linkmtu=1300 rump.ndp -n -i shmif0
	atf_check -s exit:0 -o not-match:fc00:1: rump.ndp -n -a
	atf_check -s exit:0 -o match:'fc00:1:' rump.ifconfig shmif0 inet6
	unset RUMP_SERVER

	rump_server_destroy_ifaces
}

ra_flush_defrouter_entries_cleanup()
{

	$DEBUG && dump
	terminate_rtadvd $PIDFILE
	cleanup
}

atf_test_case ra_delete_address cleanup
ra_delete_address_head()
{

	atf_set "descr" "Tests for deleting auto-configured address"
	atf_set "require.progs" "${RUMP_PROGS}"
}

ra_delete_address_body()
{

	rump_server_fs_start $RUMPSRV netinet6
	rump_server_start $RUMPCLI netinet6

	setup_shmif0 ${RUMPSRV} ${IP6SRV}
	setup_shmif0 ${RUMPCLI} ${IP6CLI}

	init_server $RUMPSRV

	create_rtadvdconfig

	RUMP_SERVER=${RUMPCLI} atf_check -s exit:0 -o match:'0 -> 1' \
		rump.sysctl -w net.inet6.ip6.accept_rtadv=1

	start_rtadvd $RUMPSRV $PIDFILE
	await_RA "${RUMPCLI}"

	check_entries $RUMPCLI $RUMPSRV $IP6SRV_PREFIX

	export RUMP_SERVER=${RUMPCLI}
	$DEBUG && rump.ifconfig shmif0
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 \
	    $(rump.ifconfig shmif0 | awk '/AUTOCONF/ {print $2}') delete
	unset RUMP_SERVER

	terminate_rtadvd $PIDFILE

	rump_server_destroy_ifaces
}

ra_delete_address_cleanup()
{

	$DEBUG && dump
	terminate_rtadvd $PIDFILE
	cleanup
}

atf_test_case ra_multiple_routers cleanup
ra_multiple_routers_head()
{

	atf_set "descr" "Tests for multiple routers"
	atf_set "require.progs" "${RUMP_PROGS}"
}

ra_multiple_routers_body()
{
	local n=

	rump_server_fs_start $RUMPSRV netinet6
	rump_server_fs_start $RUMPSRV3 netinet6
	rump_server_start $RUMPCLI netinet6

	setup_shmif0 ${RUMPSRV} ${IP6SRV}
	setup_shmif0 ${RUMPSRV3} ${IP6SRV3}
	setup_shmif0 ${RUMPCLI} ${IP6CLI}

	init_server $RUMPSRV
	init_server $RUMPSRV3

	create_rtadvdconfig

	RUMP_SERVER=${RUMPCLI} atf_check -s exit:0 -o match:'0 -> 1' \
		rump.sysctl -w net.inet6.ip6.accept_rtadv=1

	start_rtadvd $RUMPSRV $PIDFILE
	start_rtadvd $RUMPSRV3 $PIDFILE3
	await_RA "${RUMPCLI}"
	await_RA "${RUMPCLI}"

	check_entries $RUMPCLI $RUMPSRV $IP6SRV_PREFIX
	check_entries $RUMPCLI $RUMPSRV3 $IP6SRV3_PREFIX

	# Two prefixes are advertised by differnt two routers
	n=$(RUMP_SERVER=$RUMPCLI rump.ndp -p | grep 'advertised by' | wc -l)
	atf_check_equal $n 2

	terminate_rtadvd $PIDFILE
	terminate_rtadvd $PIDFILE3

	rump_server_destroy_ifaces
}

ra_multiple_routers_cleanup()
{

	$DEBUG && dump
	terminate_rtadvd $PIDFILE
	terminate_rtadvd $PIDFILE3
	cleanup
}

atf_test_case ra_multiple_routers_single_prefix cleanup
ra_multiple_routers_single_prefix_head()
{

	atf_set "descr" "Tests for multiple routers with a single prefix"
	atf_set "require.progs" "${RUMP_PROGS}"
}

ra_multiple_routers_single_prefix_body()
{
	local n=

	rump_server_fs_start $RUMPSRV netinet6
	rump_server_fs_start $RUMPSRV1_2 netinet6
	rump_server_start $RUMPCLI netinet6

	setup_shmif0 ${RUMPSRV} ${IP6SRV}
	setup_shmif0 ${RUMPSRV1_2} ${IP6SRV1_2}
	setup_shmif0 ${RUMPCLI} ${IP6CLI}

	init_server $RUMPSRV
	init_server $RUMPSRV1_2

	create_rtadvdconfig

	RUMP_SERVER=${RUMPCLI} atf_check -s exit:0 -o match:'0 -> 1' \
		rump.sysctl -w net.inet6.ip6.accept_rtadv=1

	start_rtadvd $RUMPSRV $PIDFILE
	start_rtadvd $RUMPSRV1_2 $PIDFILE1_2
	await_RA "${RUMPCLI}"
	await_RA "${RUMPCLI}"

	check_entries $RUMPCLI $RUMPSRV $IP6SRV_PREFIX
	check_entries $RUMPCLI $RUMPSRV1_2 $IP6SRV_PREFIX

	export RUMP_SERVER=$RUMPCLI
	# One prefix is advertised by differnt two routers
	n=$(rump.ndp -p |grep 'advertised by' |wc -l)
	atf_check_equal $n 1
	unset RUMP_SERVER

	terminate_rtadvd $PIDFILE
	terminate_rtadvd $PIDFILE1_2

	rump_server_destroy_ifaces
}

ra_multiple_routers_single_prefix_cleanup()
{

	$DEBUG && dump
	terminate_rtadvd $PIDFILE
	terminate_rtadvd $PIDFILE1_2
	cleanup
}

atf_test_case ra_multiple_routers_maxifprefixes cleanup
ra_multiple_routers_maxifprefixes_head()
{

	atf_set "descr" "Tests for exceeding the number of maximum prefixes"
	atf_set "require.progs" "${RUMP_PROGS}"
}

ra_multiple_routers_maxifprefixes_body()
{
	local n=

	rump_server_fs_start $RUMPSRV netinet6
	rump_server_fs_start $RUMPSRV3 netinet6
	rump_server_fs_start $RUMPSRV4 netinet6
	rump_server_start $RUMPCLI netinet6

	setup_shmif0 ${RUMPSRV} ${IP6SRV}
	setup_shmif0 ${RUMPSRV3} ${IP6SRV3}
	setup_shmif0 ${RUMPSRV4} ${IP6SRV4}
	setup_shmif0 ${RUMPCLI} ${IP6CLI}

	init_server $RUMPSRV
	init_server $RUMPSRV3
	init_server $RUMPSRV4

	create_rtadvdconfig

	RUMP_SERVER=${RUMPCLI} atf_check -s exit:0 -o match:'0.->.1' \
	    rump.sysctl -w net.inet6.ip6.accept_rtadv=1
	# Limit the maximum number of prefix entries to 2
	RUMP_SERVER=${RUMPCLI} atf_check -s exit:0 -o match:'16 -> 2' \
	    rump.sysctl -w net.inet6.ip6.maxifprefixes=2

	start_rtadvd $RUMPSRV $PIDFILE
	start_rtadvd $RUMPSRV3 $PIDFILE3
	await_RA "${RUMPCLI}"
	await_RA "${RUMPCLI}"

	check_entries $RUMPCLI $RUMPSRV $IP6SRV_PREFIX
	check_entries $RUMPCLI $RUMPSRV3 $IP6SRV3_PREFIX

	start_rtadvd $RUMPSRV4 $PIDFILE4
	await_RA "${RUMPCLI}"

	$DEBUG && RUMP_SERVER="${RUMPCLI}" dump_entries
	# There should remain two prefixes
	n=$(RUMP_SERVER=${RUMPCLI} rump.ndp -p | grep 'advertised by' | wc -l)
	atf_check_equal $n 2
	# TODO check other conditions

	terminate_rtadvd $PIDFILE
	terminate_rtadvd $PIDFILE3
	terminate_rtadvd $PIDFILE4

	rump_server_destroy_ifaces
}

ra_multiple_routers_maxifprefixes_cleanup()
{

	$DEBUG && dump
	terminate_rtadvd $PIDFILE
	terminate_rtadvd $PIDFILE3
	terminate_rtadvd $PIDFILE4
	cleanup
}

atf_test_case ra_temporary_address cleanup
ra_temporary_address_head()
{

	atf_set "descr" "Tests for IPv6 temporary address"
	atf_set "require.progs" "${RUMP_PROGS}"
}

check_echo_request_pkt()
{
	local pkt="$2 > $3: .+ echo request"

	extract_new_packets $1 > ./out
	$DEBUG && echo "$pkt"
	$DEBUG && cat ./out
	atf_check -s exit:0 -o match:"$pkt" cat ./out
}

ra_temporary_address_body()
{
	local ip_auto= ip_temp=

	rump_server_fs_start $RUMPSRV netinet6
	rump_server_start $RUMPCLI netinet6

	setup_shmif0 $RUMPSRV $IP6SRV
	init_server $RUMPSRV
	setup_shmif0 $RUMPCLI $IP6CLI

	RUMP_SERVER=$RUMPCLI atf_check -s exit:0 -o match:'0 -> 1' \
	    rump.sysctl -w net.inet6.ip6.accept_rtadv=1
	RUMP_SERVER=$RUMPCLI atf_check -s exit:0 -o match:'0 -> 1' \
	    rump.sysctl -w net.inet6.ip6.use_tempaddr=1

	create_rtadvdconfig
	start_rtadvd $RUMPSRV $PIDFILE
	await_RA "${RUMPCLI}"

	check_entries $RUMPCLI $RUMPSRV $IP6SRV_PREFIX

	export RUMP_SERVER=$RUMPCLI

	# Check temporary address
	atf_check -s exit:0 \
	    -o match:"$IP6SRV_PREFIX.+<(TENTATIVE,)?AUTOCONF,TEMPORARY>" \
	    rump.ifconfig shmif0 inet6

	#
	# Testing net.inet6.ip6.prefer_tempaddr
	#
	atf_check -s exit:0 rump.ifconfig -w 10
	$DEBUG && rump.ifconfig shmif0
	ip_auto=$(rump.ifconfig shmif0 |
	    awk '/<AUTOCONF>/ {sub(/\/[0-9]*/, ""); print $2;}')
	ip_temp=$(rump.ifconfig shmif0 |
	    awk '/<AUTOCONF,TEMPORARY>/ {sub(/\/[0-9]*/, ""); print $2;}')
	$DEBUG && echo "AUTO=$ip_auto TEMP=$ip_temp"

	# Ignore old packets
	extract_new_packets bus1 > /dev/null

	atf_check -s exit:0 -o ignore rump.ping6 -n -X 2 -c 1 $IP6SRV
	# autoconf (non-temporal) address should be used as the source address
	check_echo_request_pkt bus1 $ip_auto $IP6SRV

	# Enable net.inet6.ip6.prefer_tempaddr
	atf_check -s exit:0 -o match:'0 -> 1' \
	    rump.sysctl -w net.inet6.ip6.prefer_tempaddr=1

	atf_check -s exit:0 -o ignore rump.ping6 -n -X 2 -c 1 $IP6SRV
	# autoconf, temporal address should be used as the source address
	check_echo_request_pkt bus1 $ip_temp $IP6SRV

	unset RUMP_SERVER

	terminate_rtadvd $PIDFILE

	rump_server_destroy_ifaces
}

ra_temporary_address_cleanup()
{

	$DEBUG && dump
	terminate_rtadvd $PIDFILE
	cleanup
}

atf_test_case ra_defrouter_expiration cleanup
ra_defrouter_expiration_head()
{

	atf_set "descr" "Tests for default router expiration"
	atf_set "require.progs" "${RUMP_PROGS}"
}

ra_defrouter_expiration_body()
{
	local expire_time=5

	rump_server_fs_start $RUMPSRV netinet6
	rump_server_start $RUMPCLI netinet6

	setup_shmif0 ${RUMPSRV} ${IP6SRV}
	setup_shmif0 ${RUMPCLI} ${IP6CLI}

	init_server $RUMPSRV

	create_rtadvdconfig_rltime $expire_time

	RUMP_SERVER=${RUMPCLI} atf_check -s exit:0 -o match:'0.->.1' \
	    rump.sysctl -w net.inet6.ip6.accept_rtadv=1

	start_rtadvd $RUMPSRV $PIDFILE
	await_RA "${RUMPCLI}"

	check_entries $RUMPCLI $RUMPSRV $IP6SRV_PREFIX

	export RUMP_SERVER=${RUMPCLI}

	# Terminate rtadvd to prevent new RA messages from coming
	terminate_rtadvd $PIDFILE

	# Wait until the default routers and prefix entries are expired
	# XXX need to work out a better way ... this is a race.
	# XXX fortunately this race usually ends with the winner we want
	# XXX (long odds on - not worth placing a bet...)
	sleep $expire_time

	$DEBUG && dump_entries

	# Give nd6_timer a chance to sweep default routers and prefix entries
	# XXX Ugh again.
	sleep 2

	$DEBUG && dump_entries
	atf_check -s exit:0 -o not-match:if=shmif0 rump.ndp -r
	atf_check -s exit:0 -o match:'No advertising router' rump.ndp -p
	atf_check -s exit:0 -o match:linkmtu=1300 rump.ndp -n -i shmif0
	atf_check -s exit:0 -o not-match:fc00:1: rump.ndp -n -a
	atf_check -s exit:0 -o match:fc00:1: rump.ifconfig shmif0 inet6
	unset RUMP_SERVER

	rump_server_destroy_ifaces
}

ra_defrouter_expiration_cleanup()
{

	$DEBUG && dump
	terminate_rtadvd $PIDFILE
	cleanup
}

atf_test_case ra_prefix_expiration cleanup
ra_prefix_expiration_head()
{

	atf_set "descr" "Tests for prefix expiration"
	atf_set "require.progs" "${RUMP_PROGS}"
}

ra_prefix_expiration_body()
{
	local expire_time=5

	rump_server_fs_start $RUMPSRV netinet6
	rump_server_start $RUMPCLI netinet6

	setup_shmif0 ${RUMPSRV} ${IP6SRV}
	setup_shmif0 ${RUMPCLI} ${IP6CLI}

	init_server $RUMPSRV

	create_rtadvdconfig_vltime "${IP6SRV_PREFIX}:" $expire_time

	RUMP_SERVER=${RUMPCLI} atf_check -s exit:0 -o match:'0 -> 1' \
	    rump.sysctl -w net.inet6.ip6.accept_rtadv=1

	start_rtadvd $RUMPSRV $PIDFILE
	await_RA "${RUMPCLI}"

	check_entries $RUMPCLI $RUMPSRV $IP6SRV_PREFIX

	export RUMP_SERVER=${RUMPCLI}

	# Terminate rtadvd to prevent new RA messages from coming
	terminate_rtadvd $PIDFILE

	# Wait until the default routers and prefix entries are expired
	# XXX need the same better way here too...
	sleep $expire_time

	$DEBUG && dump_entries

	# Give nd6_timer a chance to sweep default routers and prefix entries
	# XXX and here
	sleep 2

	$DEBUG && dump_entries
	atf_check -s exit:0 -o match:if=shmif0 rump.ndp -r
	atf_check -s exit:0 -o empty rump.ndp -p
	atf_check -s exit:0 -o match:linkmtu=1300 rump.ndp -n -i shmif0
	atf_check -s exit:0 -o match:"$ONEDAYISH S R" -o not-match:fc00:1: \
		rump.ndp -n -a
	atf_check -s exit:0 -o not-match:fc00:1: rump.ifconfig shmif0 inet6
	unset RUMP_SERVER

	rump_server_destroy_ifaces
}

ra_prefix_expiration_cleanup()
{

	$DEBUG && dump
	terminate_rtadvd $PIDFILE
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case ra_basic
	atf_add_test_case ra_flush_prefix_entries
	atf_add_test_case ra_flush_defrouter_entries
	atf_add_test_case ra_delete_address
	atf_add_test_case ra_multiple_routers
	atf_add_test_case ra_multiple_routers_single_prefix
	atf_add_test_case ra_multiple_routers_maxifprefixes
	atf_add_test_case ra_temporary_address
	atf_add_test_case ra_defrouter_expiration
	atf_add_test_case ra_prefix_expiration
}
