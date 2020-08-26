#!/bin/sh

RUMPLIBS="-lrumpnet -lrumpnet_net -lrumpnet_netinet \
    -lrumpdev -lrumpvfs -lrumpdev_opencrypto -lrumpkern_z \
    -lrumpkern_crypto -lrumpnet_wg -lrumpnet_netinet6"
HIJACKING="env LD_PRELOAD=/usr/lib/librumphijack.so \
    RUMPHIJACK=path=/rump,socket=all:nolocal,sysctl=yes"

if [ $(whoami) != root ]; then
	echo run as root
	exit 1
fi

usage()
{
	local prog=$(basename $0)

	echo "Usage:"
	echo -e "\t$prog <id> create"
	echo -e "\t$prog <id> destroy"
	echo -e "\t$prog <id> ifconfig [args...]"
	echo -e "\t$prog <id> wgconfig [args...]"
	echo
	echo "<id>: must be a numeric number as it's used as an interface ID"
	exit 1
}

if [ $# -lt 2 ]; then
	usage
fi

ifid=$1
cmd=$2
shift;shift
args="$*"

tun=tun$ifid
wg=wg$ifid

sock=/var/run/wg_rump.${ifid}.sock
export RUMP_SERVER=unix://$sock

case $cmd in
create)
	rump_server $RUMPLIBS unix://$sock
	rump.ifconfig $wg create
	rump.ifconfig $wg linkstr $tun
	;;
destroy)
	rump.halt
	;;
ifconfig)
	rump.ifconfig $args
	;;
wgconfig)
	$HIJACKING wgconfig $args
	;;
debug)
	$HIJACKING $args
	;;
*)
	usage
esac
