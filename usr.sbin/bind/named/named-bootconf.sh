#!/bin/sh
#
# $NetBSD: named-bootconf.sh,v 1.4 1998/12/14 15:39:12 lukem Exp $
#
# Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Matthias Scheler.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the NetBSD
#	Foundation, Inc. and its contributors.
# 4. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
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

if [ ${OPTIONFILE-X} = X ]; then
	OPTIONFILE=/tmp/.options.`date +%s`.$$
	ZONEFILE=/tmp/.zones.`date +%s`.$$
	export OPTIONFILE ZONEFILE
	touch $OPTIONFILE $ZONEFILE
	DUMP=1
else
	DUMP=0
fi

while read CMD ARGS; do
	case $CMD in
	\; )
		echo \# $ARGS
		;;
	cache )
		set - X $ARGS
		shift
		if [ $# -eq 2 ]; then
			(echo ""
			echo "zone \"$1\" {"
			echo "	type hint;"
			echo "	file \"$2\";"
			echo "};") >>$ZONEFILE
		fi
		;;
	directory )
		set - X $ARGS
		shift
		if [ $# -eq 1 -a -d $1 ]; then
			echo "	directory \"$1\";" >>$OPTIONFILE
			DIRECTORY=$1
			export DIRECTORY
		fi
		;; 
	forwarders )
		(echo "	forwarders {"
		for ARG in $ARGS; do
			echo "		$ARG;"
		done
		echo "	};") >>$OPTIONFILE
		;;
	include )
		if [ "$ARGS" != "" ]; then
			(cd ${DIRECTORY-.}; cat $ARGS) | $0
		fi
		;;
	limit )
		set - X $ARGS
		shift
		if [ $# -eq 2 ]; then
			case $1 in
			datasize | files | transfers-in | transfers-per-ns )
				echo "	$1 $2;" >>$OPTIONFILE
				;;
			esac
		fi
		;;
	options )
		for ARG in $ARGS; do
			case $ARG in
			fake-iquery )
				echo "	fake-iquery yes;" >>$OPTIONFILE
				;;
			forward-only )
				echo "	forward only;" >>$OPTIONFILE
				;;
			no-fetch-glue )
				echo "	fetch-glue no;" >>$OPTIONFILE
				;;
			no-recursion )
				echo "	recursion no;" >>$OPTIONFILE
				;;
			esac
		done
		;;
	primary|primary/* )
		case $CMD in
		primary/CHAOS )
			class="chaos "
			;;
		primary/HS )
			class="hesiod "
			;;
		esac
		set - X $ARGS
		shift
		if [ $# -eq 2 ]; then
			(echo ""
			echo "zone \"$1\" ${class}{"
			echo "	type master;"
			echo "	file \"$2\";"
			echo "};") >>$ZONEFILE
		fi
		;;
	secondary|secondary/* )
		case $CMD in
		secondary/CHAOS )
			class="chaos "
			;;
		secondary/HS )
			class="hesiod "
			;;
		esac
		set - X $ARGS
		shift
		if [ $# -gt 2 ]; then
			ZONE=$1
			shift
			PRIMARIES=$1
			while [ $# -gt 2 ]; do
				shift
				PRIMARIES="$PRIMARIES $1"
			done
			(echo ""
			echo "zone \"$ZONE\" ${class}{"
			echo "	type slave;"
			echo "	file \"$2\";"
			echo "	masters {"
			for PRIMARY in $PRIMARIES; do
				echo "		$PRIMARY;"
			done
			echo "	};"
			echo "};") >>$ZONEFILE
		fi
		;;
	slave )
		echo "	forward only;" >>$OPTIONFILE
		;;
	sortlist )
		(echo "	topology {"
		for ARG in $ARGS; do
			case $ARG in
			*.0.0.0 )
				echo "		$ARG/8;"
				;;
			*.0.0 )
				echo "		$ARG/16;"
				;;
			*.0 )
				echo "		$ARG/24;"
				;;
			* )
				echo "		$ARG;"
				;;
			esac
		done
		echo "	};") >>$OPTIONFILE
		;;
	tcplist | xfrnets )
		(echo "	allow-transfer {"
		for ARG in $ARGS; do
			case $ARG in
			*.0.0.0 )
				echo "		$ARG/8;"
				;;
			*.0.0 )
				echo "		$ARG/16;"
				;;
			*.0 )
				echo "		$ARG/24;"
				;;
			* )
				echo "		$ARG;"
				;;
			esac
		done
		echo "	};") >>$OPTIONFILE
		;;
	esac
done

if [ $DUMP -eq 1 ]; then
	if [ -s $OPTIONFILE ]; then
		echo ""
		echo "options {"
		cat $OPTIONFILE
		echo "};"
	fi
	cat $ZONEFILE

	rm -f $OPTIONFILE $ZONEFILE
fi

exit 0
