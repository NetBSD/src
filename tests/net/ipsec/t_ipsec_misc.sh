#	$NetBSD: t_ipsec_misc.sh,v 1.23 2019/07/23 04:31:25 ozaki-r Exp $
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

SOCK_LOCAL=unix://ipsec_local
SOCK_PEER=unix://ipsec_peer
BUS=./bus_ipsec

DEBUG=${DEBUG:-true}

setup_sasp()
{
	local proto=$1
	local algo_args="$2"
	local ip_local=$3
	local ip_peer=$4
	local lifetime=$5
	local update=$6
	local tmpfile=./tmp
	local saadd=add
	local saadd_algo_args="$algo_args"
	local extra=

	if [ "$update" = getspi ]; then
		saadd=getspi
		saadd_algo_args=
	fi

	if [ "$update" = sa -o "$update" = getspi ]; then
		extra="update $ip_local $ip_peer $proto 10000 $algo_args;
		       update $ip_peer $ip_local $proto 10001 $algo_args;"
	elif [ "$update" = sp ]; then
		extra="spdupdate $ip_local $ip_peer any -P out ipsec $proto/transport//require;"
	fi

	export RUMP_SERVER=$SOCK_LOCAL
	cat > $tmpfile <<-EOF
	$saadd $ip_local $ip_peer $proto 10000 -lh $lifetime -ls $lifetime $saadd_algo_args;
	$saadd $ip_peer $ip_local $proto 10001 -lh $lifetime -ls $lifetime $saadd_algo_args;
	spdadd $ip_local $ip_peer any -P out ipsec $proto/transport//require;
	$extra
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	# XXX it can be expired if $lifetime is very short
	#check_sa_entries $SOCK_LOCAL $ip_local $ip_peer

	if [ "$update" = sp ]; then
		extra="spdupdate $ip_peer $ip_local any -P out ipsec $proto/transport//require;"
	fi

	export RUMP_SERVER=$SOCK_PEER
	cat > $tmpfile <<-EOF
	$saadd $ip_local $ip_peer $proto 10000 -lh $lifetime -ls $lifetime $saadd_algo_args;
	$saadd $ip_peer $ip_local $proto 10001 -lh $lifetime -ls $lifetime $saadd_algo_args;
	spdadd $ip_peer $ip_local any -P out ipsec $proto/transport//require;
	$extra
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	# XXX it can be expired if $lifetime is very short
	#check_sa_entries $SOCK_PEER $ip_local $ip_peer
}

test_sad_disapper_until()
{
	local time=$1
	local check_dead_sa=$2
	local setkey_opts=
	local n=$time
	local tmpfile=./__tmp
	local sock= ok=

	if $check_dead_sa; then
		setkey_opts="-D -a"
	else
		setkey_opts="-D"
	fi

	while [ $n -ne 0 ]; do
		ok=0
		sleep 1
		for sock in $SOCK_LOCAL $SOCK_PEER; do
			export RUMP_SERVER=$sock
			$HIJACKING setkey $setkey_opts > $tmpfile
			$DEBUG && cat $tmpfile
			if grep -q 'No SAD entries.' $tmpfile; then
				ok=$((ok + 1))
			fi
		done
		if [ $ok -eq 2 ]; then
			return
		fi

		n=$((n - 1))
	done

	atf_fail "SAs didn't disappear after $time sec."
}

test_ipsec4_lifetime()
{
	local proto=$1
	local algo=$2
	local ip_local=10.0.0.1
	local ip_peer=10.0.0.2
	local outfile=./out
	local proto_cap=$(echo $proto | tr 'a-z' 'A-Z')
	local algo_args="$(generate_algo_args $proto $algo)"
	local lifetime=3
	local buffertime=2

	rump_server_crypto_start $SOCK_LOCAL netipsec
	rump_server_crypto_start $SOCK_PEER netipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_PEER shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_local/24
	#atf_check -s exit:0 -o ignore rump.sysctl -w net.key.debug=0xff

	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_peer/24
	#atf_check -s exit:0 -o ignore rump.sysctl -w net.key.debug=0xff

	extract_new_packets $BUS > $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer

	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:"$ip_local > $ip_peer: ICMP echo request" \
	    cat $outfile
	atf_check -s exit:0 -o match:"$ip_peer > $ip_local: ICMP echo reply" \
	    cat $outfile

	# Set up SAs with lifetime 1 sec.
	setup_sasp $proto "$algo_args" $ip_local $ip_peer 1

	# Check the SAs have been expired
	test_sad_disapper_until $((1 + $buffertime)) false

	# Clean up SPs
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o empty $HIJACKING setkey -F -P
	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 -o empty $HIJACKING setkey -F -P

	# Set up SAs with lifetime with $lifetime
	setup_sasp $proto "$algo_args" $ip_local $ip_peer $lifetime

	# Use the SAs; this will create a reference from an SP to an SA
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer

	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:"$ip_local > $ip_peer: $proto_cap" \
	    cat $outfile
	atf_check -s exit:0 -o match:"$ip_peer > $ip_local: $proto_cap" \
	    cat $outfile

	# Check the SAs have been expired
	test_sad_disapper_until $((lifetime + $buffertime)) true

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s not-exit:0 -o match:'0 packets received' \
	    rump.ping -c 1 -n -w 1 $ip_peer

	test_flush_entries $SOCK_LOCAL
	test_flush_entries $SOCK_PEER
}

test_ipsec6_lifetime()
{
	local proto=$1
	local algo=$2
	local ip_local=fd00::1
	local ip_peer=fd00::2
	local outfile=./out
	local proto_cap=$(echo $proto | tr 'a-z' 'A-Z')
	local algo_args="$(generate_algo_args $proto $algo)"
	local lifetime=3
	local buffertime=2

	rump_server_crypto_start $SOCK_LOCAL netinet6 netipsec
	rump_server_crypto_start $SOCK_PEER netinet6 netipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_PEER shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_local

	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 rump.sysctl -q -w net.inet6.ip6.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $ip_peer

	extract_new_packets $BUS > $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -n -X 3 $ip_peer

	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:"$ip_local > $ip_peer: ICMP6, echo request" \
	    cat $outfile
	atf_check -s exit:0 -o match:"$ip_peer > $ip_local: ICMP6, echo reply" \
	    cat $outfile

	# Set up SAs with lifetime 1 sec.
	setup_sasp $proto "$algo_args" $ip_local $ip_peer 1

	# Check the SAs have been expired
	test_sad_disapper_until $((1 + $buffertime)) false

	# Clean up SPs
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o empty $HIJACKING setkey -F -P
	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 -o empty $HIJACKING setkey -F -P

	# Set up SAs with lifetime with $lifetime
	setup_sasp $proto "$algo_args" $ip_local $ip_peer $lifetime

	# Use the SAs; this will create a reference from an SP to an SA
	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping6 -c 1 -n -X 3 $ip_peer

	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:"$ip_local > $ip_peer: $proto_cap" \
	    cat $outfile
	atf_check -s exit:0 -o match:"$ip_peer > $ip_local: $proto_cap" \
	    cat $outfile

	# Check the SAs have been expired
	test_sad_disapper_until $((lifetime + $buffertime)) true

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s not-exit:0 -o match:'0 packets received' \
	    rump.ping6 -c 1 -n -X 1 $ip_peer

	test_flush_entries $SOCK_LOCAL
	test_flush_entries $SOCK_PEER
}

test_lifetime_common()
{
	local ipproto=$1
	local proto=$2
	local algo=$3

	if [ $ipproto = ipv4 ]; then
		test_ipsec4_lifetime $proto $algo
	else
		test_ipsec6_lifetime $proto $algo
	fi
}

add_test_lifetime()
{
	local ipproto=$1
	local proto=$2
	local algo=$3
	local _algo=$(echo $algo | sed 's/-//g')
	local name= desc=

	name="ipsec_lifetime_${ipproto}_${proto}_${_algo}"
	desc="Tests of lifetime of IPsec ($ipproto) with $proto ($algo)"

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server setkey
	    }
	    ${name}_body() {
	        test_lifetime_common $ipproto $proto $algo
	        rump_server_destroy_ifaces
	    }
	    ${name}_cleanup() {
	        \$DEBUG && dump
	        cleanup
	    }
	"
	atf_add_test_case ${name}
}

test_update()
{
	local proto=$1
	local algo=$2
	local update=$3
	local ip_local=10.0.0.1
	local ip_peer=10.0.0.2
	local algo_args="$(generate_algo_args $proto $algo)"
	local proto_cap=$(echo $proto | tr 'a-z' 'A-Z')
	local outfile=./out

	rump_server_crypto_start $SOCK_LOCAL netipsec
	rump_server_crypto_start $SOCK_PEER netipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_PEER shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_local/24

	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_peer/24

	setup_sasp $proto "$algo_args" $ip_local $ip_peer 100 $update

	extract_new_packets $BUS > $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer

	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:"$ip_local > $ip_peer: $proto_cap" \
	    cat $outfile
	atf_check -s exit:0 -o match:"$ip_peer > $ip_local: $proto_cap" \
	    cat $outfile
}

add_test_update()
{
	local proto=$1
	local algo=$2
	local update=$3
	local _update=$(echo $update |tr 'a-z' 'A-Z')
	local _algo=$(echo $algo | sed 's/-//g')
	local name= desc=

	desc="Tests trying to udpate $_update of $proto ($algo)"
	name="ipsec_update_${update}_${proto}_${_algo}"

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server setkey
	    }
	    ${name}_body() {
	        test_update $proto $algo $update
	        rump_server_destroy_ifaces
	    }
	    ${name}_cleanup() {
	        \$DEBUG && dump
	        cleanup
	    }
	"
	atf_add_test_case ${name}
}

test_getspi_update()
{
	local proto=$1
	local algo=$2
	local ip_local=10.0.0.1
	local ip_peer=10.0.0.2
	local algo_args="$(generate_algo_args $proto $algo)"
	local proto_cap=$(echo $proto | tr 'a-z' 'A-Z')
	local outfile=./out

	rump_server_crypto_start $SOCK_LOCAL netipsec
	rump_server_crypto_start $SOCK_PEER netipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_PEER shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_local/24

	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_peer/24

	setup_sasp $proto "$algo_args" $ip_local $ip_peer 100 getspi

	extract_new_packets $BUS > $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer

	extract_new_packets $BUS > $outfile
	atf_check -s exit:0 -o match:"$ip_local > $ip_peer: $proto_cap" \
	    cat $outfile
	atf_check -s exit:0 -o match:"$ip_peer > $ip_local: $proto_cap" \
	    cat $outfile
}

add_test_getspi_update()
{
	local proto=$1
	local algo=$2
	local _algo=$(echo $algo | sed 's/-//g')
	local name= desc=

	desc="Tests trying to getspi and udpate SA of $proto ($algo)"
	name="ipsec_getspi_update_sa_${proto}_${_algo}"

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server setkey
	    }
	    ${name}_body() {
	        test_getspi_update $proto $algo
	        rump_server_destroy_ifaces
	    }
	    ${name}_cleanup() {
	        \$DEBUG && dump
	        cleanup
	    }
	"
	atf_add_test_case ${name}
}

add_sa()
{
	local proto=$1
	local algo_args="$2"
	local ip_local=$3
	local ip_peer=$4
	local lifetime=$5
	local spi=$6
	local tmpfile=./tmp
	local extra=

	export RUMP_SERVER=$SOCK_LOCAL
	cat > $tmpfile <<-EOF
	add $ip_local $ip_peer $proto $((spi)) -lh $lifetime -ls $lifetime $algo_args;
	add $ip_peer $ip_local $proto $((spi + 1)) -lh $lifetime -ls $lifetime $algo_args;
	$extra
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D
	# XXX it can be expired if $lifetime is very short
	#check_sa_entries $SOCK_LOCAL $ip_local $ip_peer

	export RUMP_SERVER=$SOCK_PEER
	cat > $tmpfile <<-EOF
	add $ip_local $ip_peer $proto $((spi)) -lh $lifetime -ls $lifetime $algo_args;
	add $ip_peer $ip_local $proto $((spi + 1)) -lh $lifetime -ls $lifetime $algo_args;
	$extra
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D
	# XXX it can be expired if $lifetime is very short
	#check_sa_entries $SOCK_PEER $ip_local $ip_peer
}

delete_sa()
{
	local proto=$1
	local ip_local=$2
	local ip_peer=$3
	local spi=$4
	local tmpfile=./tmp
	local extra=

	export RUMP_SERVER=$SOCK_LOCAL
	cat > $tmpfile <<-EOF
	delete $ip_local $ip_peer $proto $((spi));
	delete $ip_peer $ip_local $proto $((spi + 1));
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D

	export RUMP_SERVER=$SOCK_PEER
	cat > $tmpfile <<-EOF
	delete $ip_local $ip_peer $proto $((spi));
	delete $ip_peer $ip_local $proto $((spi + 1));
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	$DEBUG && $HIJACKING setkey -D
}

check_packet_spi()
{
	local outfile=$1
	local ip_local=$2
	local ip_peer=$3
	local proto=$4
	local spi=$5
	local spistr=

	$DEBUG && cat $outfile
	spistr=$(printf "%08x" $spi)
	atf_check -s exit:0 \
	    -o match:"$ip_local > $ip_peer: $proto_cap\(spi=0x$spistr," \
	    cat $outfile
	spistr=$(printf "%08x" $((spi + 1)))
	atf_check -s exit:0 \
	    -o match:"$ip_peer > $ip_local: $proto_cap\(spi=0x$spistr," \
	    cat $outfile
}

wait_sa_disappeared()
{
	local spi=$1
	local i=

	export RUMP_SERVER=$SOCK_LOCAL
	for i in $(seq 1 10); do
		$HIJACKING setkey -D |grep -q "spi=$spi"
		[ $? != 0 ] && break
		sleep 1
	done
	if [ $i -eq 10 ]; then
		atf_fail "SA (spi=$spi) didn't disappear in 10s"
	fi
	export RUMP_SERVER=$SOCK_PEER
	for i in $(seq 1 10); do
		$HIJACKING setkey -D |grep -q "spi=$spi"
		[ $? != 0 ] && break
		sleep 1
	done
	if [ $i -eq 10 ]; then
		atf_fail "SA (spi=$spi) didn't disappear in 10s"
	fi
}

test_spi()
{
	local proto=$1
	local algo=$2
	local preferred=$3
	local method=$4
	local ip_local=10.0.0.1
	local ip_peer=10.0.0.2
	local algo_args="$(generate_algo_args $proto $algo)"
	local proto_cap=$(echo $proto | tr 'a-z' 'A-Z')
	local outfile=./out
	local spistr=
	local longtime= shorttime=

	if [ $method = timeout -a $preferred = new ]; then
		skip_if_qemu
	fi

	if [ $method = delete ]; then
		shorttime=100
		longtime=100
	else
		shorttime=3
		longtime=6
	fi

	rump_server_crypto_start $SOCK_LOCAL netipsec
	rump_server_crypto_start $SOCK_PEER netipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_PEER shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_local/24
	if [ $preferred = old ]; then
		atf_check -s exit:0 rump.sysctl -q -w net.key.prefered_oldsa=1
	fi

	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_peer/24
	if [ $preferred = old ]; then
		atf_check -s exit:0 rump.sysctl -q -w net.key.prefered_oldsa=1
	fi

	setup_sasp $proto "$algo_args" $ip_local $ip_peer 100

	extract_new_packets $BUS > $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer
	extract_new_packets $BUS > $outfile
	check_packet_spi $outfile $ip_local $ip_peer $proto_cap 10000

	# Add a new SA with a different SPI
	add_sa $proto "$algo_args" $ip_local $ip_peer $longtime 10010

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer
	extract_new_packets $BUS > $outfile
	if [ $preferred = old ]; then
		check_packet_spi $outfile $ip_local $ip_peer $proto_cap 10000
	else
		# The new SA is preferred
		check_packet_spi $outfile $ip_local $ip_peer $proto_cap 10010
	fi

	# Add another SA with a different SPI
	add_sa $proto "$algo_args" $ip_local $ip_peer $shorttime 10020

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer
	extract_new_packets $BUS > $outfile
	if [ $preferred = old ]; then
		check_packet_spi $outfile $ip_local $ip_peer $proto_cap 10000
	else
		# The newest SA is preferred
		check_packet_spi $outfile $ip_local $ip_peer $proto_cap 10020
	fi

	if [ $method = delete ]; then
		delete_sa $proto $ip_local $ip_peer 10020
	else
		wait_sa_disappeared 10020
	fi

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer
	extract_new_packets $BUS > $outfile
	if [ $preferred = old ]; then
		check_packet_spi $outfile $ip_local $ip_peer $proto_cap 10000
	else
		# The newest one is removed and the second one is used
		check_packet_spi $outfile $ip_local $ip_peer $proto_cap 10010
	fi

	if [ $method = delete ]; then
		delete_sa $proto $ip_local $ip_peer 10010
	else
		wait_sa_disappeared 10010
	fi

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer
	extract_new_packets $BUS > $outfile
	if [ $preferred = old ]; then
		check_packet_spi $outfile $ip_local $ip_peer $proto_cap 10000
	else
		# The second one is removed and the original one is used
		check_packet_spi $outfile $ip_local $ip_peer $proto_cap 10000
	fi
}

add_test_spi()
{
	local proto=$1
	local algo=$2
	local preferred=$3
	local method=$4
	local _algo=$(echo $algo | sed 's/-//g')
	local name= desc=

	desc="Tests SAs with different SPIs of $proto ($algo) ($preferred SA preferred) ($method)"
	name="ipsec_spi_${proto}_${_algo}_preferred_${preferred}_${method}"

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server setkey
	    }
	    ${name}_body() {
	        test_spi $proto $algo $preferred $method
	        rump_server_destroy_ifaces
	    }
	    ${name}_cleanup() {
	        \$DEBUG && dump
	        cleanup
	    }
	"
	atf_add_test_case ${name}
}

setup_sp()
{
	local proto=$1
	local algo_args="$2"
	local ip_local=$3
	local ip_peer=$4
	local tmpfile=./tmp

	export RUMP_SERVER=$SOCK_LOCAL
	cat > $tmpfile <<-EOF
	spdadd $ip_local $ip_peer any -P out ipsec $proto/transport//require;
	spdadd $ip_peer $ip_local any -P in ipsec $proto/transport//require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	check_sp_entries $SOCK_LOCAL $ip_local $ip_peer

	export RUMP_SERVER=$SOCK_PEER
	cat > $tmpfile <<-EOF
	spdadd $ip_peer $ip_local any -P out ipsec $proto/transport//require;
	spdadd $ip_local $ip_peer any -P in ipsec $proto/transport//require;
	EOF
	$DEBUG && cat $tmpfile
	atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
	check_sp_entries $SOCK_PEER $ip_peer $ip_local
}

test_nosa()
{
	local proto=$1
	local algo=$2
	local update=$3
	local ip_local=10.0.0.1
	local ip_peer=10.0.0.2
	local algo_args="$(generate_algo_args $proto $algo)"
	local proto_cap=$(echo $proto | tr 'a-z' 'A-Z')
	local outfile=./out

	rump_server_crypto_start $SOCK_LOCAL netipsec
	rump_server_crypto_start $SOCK_PEER netipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_PEER shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_local/24

	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_peer/24

	setup_sp $proto "$algo_args" $ip_local $ip_peer

	extract_new_packets $BUS > $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	# It doesn't work because there is no SA
	atf_check -s not-exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer
}

add_test_nosa()
{
	local proto=$1
	local algo=$2
	local _algo=$(echo $algo | sed 's/-//g')
	local name= desc=

	desc="Tests SPs with no relevant SAs with $proto ($algo)"
	name="ipsec_nosa_${proto}_${_algo}"

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server setkey
	    }
	    ${name}_body() {
	        test_nosa $proto $algo
	        rump_server_destroy_ifaces
	    }
	    ${name}_cleanup() {
	        \$DEBUG && dump
	        cleanup
	    }
	"
	atf_add_test_case ${name}
}

test_multiple_sa()
{
	local proto=$1
	local algo=$2
	local update=$3
	local ip_local=10.0.0.1
	local ip_peer=10.0.0.2
	local ip_peer2=10.0.0.3
	local algo_args="$(generate_algo_args $proto $algo)"
	local proto_cap=$(echo $proto | tr 'a-z' 'A-Z')
	local outfile=./out

	rump_server_crypto_start $SOCK_LOCAL netipsec
	rump_server_crypto_start $SOCK_PEER netipsec
	rump_server_add_iface $SOCK_LOCAL shmif0 $BUS
	rump_server_add_iface $SOCK_PEER shmif0 $BUS

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_local/24

	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 rump.sysctl -q -w net.inet.ip.dad_count=0
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_peer/24
	atf_check -s exit:0 rump.ifconfig shmif0 $ip_peer2/24 alias

	setup_sp $proto "$algo_args" "$ip_local" "0.0.0.0/0"

	extract_new_packets $BUS > $outfile

	export RUMP_SERVER=$SOCK_LOCAL
	# There is no SA, so ping should fail
	atf_check -s not-exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer
	atf_check -s not-exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer2

	add_sa $proto "$algo_args" $ip_local $ip_peer 100 10000

	export RUMP_SERVER=$SOCK_LOCAL
	# There is only an SA for $ip_peer, so ping to $ip_peer2 should fail
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer
	atf_check -s not-exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer2

	add_sa $proto "$algo_args" $ip_local $ip_peer2 100 10010

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer
	atf_check -s exit:0 -o ignore rump.ping -c 1 -n -w 3 $ip_peer2

	export RUMP_SERVER=$SOCK_LOCAL
	atf_check -s exit:0 -o match:"$proto/transport//require" \
	    $HIJACKING setkey -D -P
	# Check if the policy isn't modified accidentally
	atf_check -s exit:0 -o not-match:"$proto/transport/.+\-.+/require" \
	    $HIJACKING setkey -D -P
	export RUMP_SERVER=$SOCK_PEER
	atf_check -s exit:0 -o match:"$proto/transport//require" \
	    $HIJACKING setkey -D -P
	# Check if the policy isn't modified accidentally
	atf_check -s exit:0 -o not-match:"$proto/transport/.+\-.+/require" \
	    $HIJACKING setkey -D -P
}

add_test_multiple_sa()
{
	local proto=$1
	local algo=$2
	local _algo=$(echo $algo | sed 's/-//g')
	local name= desc=

	desc="Tests multiple SAs with $proto ($algo)"
	name="ipsec_multiple_sa_${proto}_${_algo}"

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server setkey
	    }
	    ${name}_body() {
	        test_multiple_sa $proto $algo
	        rump_server_destroy_ifaces
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
	local algo=

	for algo in $ESP_ENCRYPTION_ALGORITHMS_MINIMUM; do
		add_test_lifetime ipv4 esp $algo
		add_test_lifetime ipv6 esp $algo
		add_test_update esp $algo sa
		add_test_update esp $algo sp
		add_test_getspi_update esp $algo
		add_test_spi esp $algo new delete
		add_test_spi esp $algo old delete
		add_test_spi esp $algo new timeout
		add_test_spi esp $algo old timeout
		add_test_nosa esp $algo
		add_test_multiple_sa esp $algo
	done
	for algo in $AH_AUTHENTICATION_ALGORITHMS_MINIMUM; do
		add_test_lifetime ipv4 ah $algo
		add_test_lifetime ipv6 ah $algo
		add_test_update ah $algo sa
		add_test_update ah $algo sp
		add_test_getspi_update ah $algo
		add_test_spi ah $algo new delete
		add_test_spi ah $algo old delete
		add_test_spi ah $algo new timeout
		add_test_spi ah $algo old timeout
		add_test_nosa ah $algo
		add_test_multiple_sa ah $algo
	done
}
