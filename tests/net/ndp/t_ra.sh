#	$NetBSD: t_ra.sh,v 1.8 2016/12/16 03:14:23 ozaki-r Exp $
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
RUMPCLI=unix://r2
IP6SRV=fc00:1::1
IP6CLI=fc00:2::2
PIDFILE=/var/run/rump.rtadvd.pid
CONFIG=./rtadvd.conf
DEBUG=${DEBUG:-true}

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

wait_term()
{
	local PIDFILE=${1}
	shift

	while [ -f ${PIDFILE} ]
	do
		sleep 0.2
	done

	return 0
}

create_rtadvdconfig()
{

	cat << _EOF > ${CONFIG}
shmif0:\
	:mtu#1300:maxinterval#4:mininterval#3:
_EOF
}

dump_entries()
{

	echo ndp -n -a
	rump.ndp -n -a
	echo ndp -p
	rump.ndp -p
	echo ndp -r
	rump.ndp -r
}

atf_test_case ra_basic cleanup
ra_basic_head()
{

	atf_set "descr" "Tests for basic functions of router advaertisement(RA)"
	atf_set "require.progs" "rump_server rump.rtadvd rump.ndp rump.ifconfig"
}

ra_basic_body()
{

	rump_server_fs_start $RUMPSRV netinet6
	rump_server_start $RUMPCLI netinet6

	setup_shmif0 ${RUMPSRV} ${IP6SRV}
	export RUMP_SERVER=${RUMPSRV}
	atf_check -s exit:0 -o match:'0.->.1' rump.sysctl -w net.inet6.ip6.forwarding=1
	export LD_PRELOAD=/usr/lib/librumphijack.so
	atf_check -s exit:0 mkdir -p /rump/var/chroot/rtadvd
	unset LD_PRELOAD
	unset RUMP_SERVER

	setup_shmif0 ${RUMPCLI} ${IP6CLI}
	export RUMP_SERVER=${RUMPCLI}
	$DEBUG && rump.ndp -n -a
	atf_check -s exit:0 -o match:'= 0' rump.sysctl net.inet6.ip6.accept_rtadv
	unset RUMP_SERVER

	create_rtadvdconfig

	export RUMP_SERVER=${RUMPSRV}
	atf_check -s exit:0 rump.rtadvd -c ${CONFIG} shmif0
	atf_check -s exit:0 sleep 3
	atf_check -s exit:0 -o ignore -e empty cat ${PIDFILE}
	unset RUMP_SERVER

	export RUMP_SERVER=${RUMPCLI}
	atf_check -s exit:0 -o empty rump.ndp -r
	atf_check -s exit:0 -o not-match:'advertised' rump.ndp -p
	atf_check -s exit:0 -o match:'linkmtu=0' rump.ndp -n -i shmif0
	atf_check -s exit:0 -o not-match:'S R' rump.ndp -n -a
	atf_check -s exit:0 -o not-match:'fc00:1:' rump.ndp -n -a
	atf_check -s exit:0 -o not-match:'fc00:1:' rump.ifconfig shmif0 inet6
	unset RUMP_SERVER

	atf_check -s exit:0 kill -TERM `cat ${PIDFILE}`
	wait_term ${PIDFILE}

	export RUMP_SERVER=${RUMPCLI}
	atf_check -s exit:0 -o match:'0.->.1' rump.sysctl -w net.inet6.ip6.accept_rtadv=1
	unset RUMP_SERVER

	export RUMP_SERVER=${RUMPSRV}
	atf_check -s exit:0 rump.rtadvd -c ${CONFIG} shmif0
	atf_check -s exit:0 sleep 3
	atf_check -s exit:0 -o ignore -e empty cat ${PIDFILE}
	unset RUMP_SERVER

	export RUMP_SERVER=${RUMPCLI}
	$DEBUG && rump.ndp -n -a
	$DEBUG && rump.ndp -r
	atf_check -s exit:0 -o match:'if=shmif0' rump.ndp -r
	atf_check -s exit:0 -o match:'advertised' rump.ndp -p
	atf_check -s exit:0 -o match:'linkmtu=1300' rump.ndp -n -i shmif0
	atf_check -s exit:0 -o match:'(23h59m|1d0h0m)..s S R' rump.ndp -n -a
	atf_check -s exit:0 -o match:'fc00:1:' rump.ndp -n -a
	atf_check -s exit:0 -o match:'fc00:1:' rump.ifconfig shmif0 inet6
	unset RUMP_SERVER

	atf_check -s exit:0 kill -TERM `cat ${PIDFILE}`
	wait_term ${PIDFILE}

	rump_server_destroy_ifaces
}

ra_basic_cleanup()
{

	if [ -f ${PIDFILE} ]; then
		kill -TERM `cat ${PIDFILE}`
		wait_term ${PIDFILE}
	fi

	$DEBUG && dump
	cleanup
}

atf_test_case ra_flush_prefix_entries cleanup
ra_flush_prefix_entries_head()
{

	atf_set "descr" "Tests for flushing prefixes (ndp -P)"
	atf_set "require.progs" "rump_server rump.rtadvd rump.ndp rump.ifconfig"
}

ra_flush_prefix_entries_body()
{

	rump_server_fs_start $RUMPSRV netinet6
	rump_server_start $RUMPCLI netinet6

	setup_shmif0 ${RUMPSRV} ${IP6SRV}
	setup_shmif0 ${RUMPCLI} ${IP6CLI}

	export RUMP_SERVER=${RUMPSRV}
	atf_check -s exit:0 -o match:'0.->.1' rump.sysctl -w net.inet6.ip6.forwarding=1
	export LD_PRELOAD=/usr/lib/librumphijack.so
	atf_check -s exit:0 mkdir -p /rump/var/chroot/rtadvd
	unset LD_PRELOAD
	unset RUMP_SERVER

	create_rtadvdconfig

	export RUMP_SERVER=${RUMPCLI}
	atf_check -s exit:0 -o match:'0.->.1' rump.sysctl -w net.inet6.ip6.accept_rtadv=1
	unset RUMP_SERVER

	export RUMP_SERVER=${RUMPSRV}
	atf_check -s exit:0 rump.rtadvd -c ${CONFIG} shmif0
	atf_check -s exit:0 sleep 3
	atf_check -s exit:0 -o ignore -e empty cat ${PIDFILE}
	unset RUMP_SERVER

	export RUMP_SERVER=${RUMPCLI}
	$DEBUG && dump_entries
	atf_check -s exit:0 -o match:'if=shmif0' rump.ndp -r
	atf_check -s exit:0 -o match:'advertised' rump.ndp -p
	atf_check -s exit:0 -o match:'linkmtu=1300' rump.ndp -n -i shmif0
	atf_check -s exit:0 -o match:'(23h59m|1d0h0m)..s S R' rump.ndp -n -a
	atf_check -s exit:0 -o match:'fc00:1:' rump.ndp -n -a
	atf_check -s exit:0 -o match:'fc00:1:' rump.ifconfig shmif0 inet6

	# Terminate rtadvd to prevent new RA messages from coming
	# Note that ifconfig down; kill -TERM doesn't work
	kill -KILL `cat ${PIDFILE}`

	# Flush all the entries in the prefix list
	atf_check -s exit:0 rump.ndp -P

	$DEBUG && dump_entries
	atf_check -s exit:0 -o match:'if=shmif0' rump.ndp -r
	atf_check -s exit:0 -o empty rump.ndp -p
	atf_check -s exit:0 -o match:'linkmtu=1300' rump.ndp -n -i shmif0
	atf_check -s exit:0 -o match:'(23h59m|1d0h0m)..s S R' rump.ndp -n -a
	atf_check -s exit:0 -o match:'fc00:1:' rump.ndp -n -a
	atf_check -s exit:0 -o not-match:'fc00:1:' rump.ifconfig shmif0 inet6
	unset RUMP_SERVER

	rump_server_destroy_ifaces
}

ra_flush_prefix_entries_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case ra_flush_defrouter_entries cleanup
ra_flush_defrouter_entries_head()
{

	atf_set "descr" "Tests for flushing default routers (ndp -R)"
	atf_set "require.progs" "rump_server rump.rtadvd rump.ndp rump.ifconfig"
}

ra_flush_defrouter_entries_body()
{

	rump_server_fs_start $RUMPSRV netinet6
	rump_server_start $RUMPCLI netinet6

	setup_shmif0 ${RUMPSRV} ${IP6SRV}
	setup_shmif0 ${RUMPCLI} ${IP6CLI}

	export RUMP_SERVER=${RUMPSRV}
	atf_check -s exit:0 -o match:'0.->.1' rump.sysctl -w net.inet6.ip6.forwarding=1
	export LD_PRELOAD=/usr/lib/librumphijack.so
	atf_check -s exit:0 mkdir -p /rump/var/chroot/rtadvd
	unset LD_PRELOAD
	unset RUMP_SERVER

	create_rtadvdconfig

	export RUMP_SERVER=${RUMPCLI}
	atf_check -s exit:0 -o match:'0.->.1' rump.sysctl -w net.inet6.ip6.accept_rtadv=1
	unset RUMP_SERVER

	export RUMP_SERVER=${RUMPSRV}
	atf_check -s exit:0 rump.rtadvd -c ${CONFIG} shmif0
	atf_check -s exit:0 sleep 3
	atf_check -s exit:0 -o ignore -e empty cat ${PIDFILE}
	unset RUMP_SERVER

	export RUMP_SERVER=${RUMPCLI}
	$DEBUG && dump_entries
	atf_check -s exit:0 -o match:'if=shmif0' rump.ndp -r
	atf_check -s exit:0 -o match:'advertised' rump.ndp -p
	atf_check -s exit:0 -o match:'linkmtu=1300' rump.ndp -n -i shmif0
	atf_check -s exit:0 -o match:'(23h59m|1d0h0m)..s S R' rump.ndp -n -a
	atf_check -s exit:0 -o match:'fc00:1:' rump.ndp -n -a
	atf_check -s exit:0 -o match:'fc00:1:' rump.ifconfig shmif0 inet6

	# Terminate rtadvd to prevent new RA messages from coming
	# Note that ifconfig down; kill -TERM doesn't work
	kill -KILL `cat ${PIDFILE}`

	# Flush all the entries in the default router list
	atf_check -s exit:0 rump.ndp -R

	$DEBUG && dump_entries
	atf_check -s exit:0 -o empty rump.ndp -r
	atf_check -s exit:0 -o match:'No advertising router' rump.ndp -p
	atf_check -s exit:0 -o match:'linkmtu=1300' rump.ndp -n -i shmif0
	atf_check -s exit:0 -o match:'(23h59m|1d0h0m)..s S R' rump.ndp -n -a
	atf_check -s exit:0 -o match:'fc00:1:' rump.ndp -n -a
	atf_check -s exit:0 -o match:'fc00:1:' rump.ifconfig shmif0 inet6
	unset RUMP_SERVER

	rump_server_destroy_ifaces
}

ra_flush_defrouter_entries_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_init_test_cases()
{

	atf_add_test_case ra_basic
	atf_add_test_case ra_flush_prefix_entries
	atf_add_test_case ra_flush_defrouter_entries
}
