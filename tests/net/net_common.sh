#	$NetBSD: net_common.sh,v 1.3 2016/11/24 09:05:16 ozaki-r Exp $
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

HIJACKING="env LD_PRELOAD=/usr/lib/librumphijack.so RUMPHIJACK=sysctl=yes"

extract_new_packets()
{
	local bus=$1
	local old=./.__old

	if [ ! -f $old ]; then
		old=/dev/null
	fi

	shmif_dumpbus -p - $bus 2>/dev/null| \
	    tcpdump -n -e -r - 2>/dev/null > ./.__new
	diff -u $old ./.__new |grep '^+' |cut -d '+' -f 2 > ./.__diff
	mv -f ./.__new ./.__old
	cat ./.__diff
}

check_route()
{
	local target=$1
	local gw=$2
	local flags=${3:-\.\+}
	local ifname=${4:-\.\+}

	target=$(echo $target |sed 's/\./\\./g')
	if [ "$gw" = "" ]; then
		gw=".+"
	else
		gw=$(echo $gw |sed 's/\./\\./g')
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
	local target=$(echo $1 |sed 's/\./\\./g')

	atf_check -s exit:0 -e ignore -o not-match:"^$target" \
	    rump.netstat -rn
}
