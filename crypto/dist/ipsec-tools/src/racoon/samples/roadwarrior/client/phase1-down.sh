#!/bin/sh

#
# sa-down.sh local configuration for a new SA
#

PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin

case `uname -s` in
NetBSD)
	DEFAULT_GW=`netstat -finet -rn | awk '($1 == "default"){print $2; exit}'`
	;;
Linux)
	DEFAULT_GW=`netstat --inet -rn | awk '($1 == "0.0.0.0"){print $2; exit}'`
	;;
esac

echo $@
echo "LOCAL_ADDR = ${LOCAL_ADDR}"
echo "LOCAL_PORT = ${LOCAL_PORT}"
echo "REMOTE_ADDR = ${REMOTE_ADDR}"
echo "REMOTE_PORT = ${REMOTE_PORT}"
echo "DEFAULT_GW = ${DEFAULT_GW}"
echo "INTERNAL_NETMASK4 = ${INTERNAL_NETMASK4}"
echo "INTERNAL_ADDR4 = ${INTERNAL_ADDR4}"
echo "INTERNAL_DNS4 = ${INTERNAL_DNS4}"

echo ${INTERNAL_ADDR4} | grep '[0-9]' > /dev/null || exit 0
echo ${INTERNAL_NETMASK4} | grep '[0-9]' > /dev/null || exit 0
echo ${DEFAULT_GW} | grep '[0-9]' > /dev/null || exit 0

if [ -f /etc/resolv.conf.bak ]; then
	rm -f /etc/resolv.conf
	mv /etc/resolv.conf.bak /etc/resolv.conf
fi

case `uname -s` in
NetBSD)
	if=`netstat -finet -rn|awk '($1 == "default"){print $7; exit}'`
	route delete default
	route delete ${REMOTE_ADDR}
	ifconfig ${if} delete ${INTERNAL_ADDR4}
	route add default ${DEFAULT_GW} -ifa ${LOCAL_ADDR}
	;;
Linux)
	if=`netstat --inet -rn|awk '($1 == "0.0.0.0"){print $8; exit}'`
	route delete default
	route delete ${REMOTE_ADDR}
	ifconfig ${if}:1 del ${INTERNAL_ADDR4}
	route add default gw ${DEFAULT_GW}

	#
	# XXX This is a workaround because Linux seems to ignore
	# the deleteall commands below. This is bad because it flushes
	# any SAD instead of flushing what needs to be flushed.
	# Someone using Linux please fix it
	#
	setkey -F
	;;
esac

LOCAL="${LOCAL_ADDR}"
REMOTE="${REMOTE_ADDR}"
if [ "x${LOCAL_PORT}" != "x500" ]; then
	# NAT-T setup
	LOCAL="${LOCAL}[${LOCAL_PORT}]"
	REMOTE="${REMOTE}[${REMOTE_PORT}]"
fi

echo "
deleteall ${REMOTE_ADDR} ${LOCAL_ADDR} esp;
deleteall ${LOCAL_ADDR} ${REMOTE_ADDR} esp; 
spddelete ${INTERNAL_ADDR4}/32[any] 0.0.0.0/0[any] any
	-P out ipsec esp/tunnel/${LOCAL}-${REMOTE}/require;
spddelete 0.0.0.0/0[any] ${INTERNAL_ADDR4}[any] any
	-P in ipsec esp/tunnel/${REMOTE}-${LOCAL}/require;
" | setkey -c

