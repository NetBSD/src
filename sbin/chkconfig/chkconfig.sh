#!/bin/sh
#
#	$NetBSD: chkconfig.sh,v 1.1 2001/03/14 03:51:47 thorpej Exp $
#
# Copyright (c) 2001 Zembu Labs, Inc.
# All rights reserved.
#
# Author: Dan Mercer <dmercer@zembu.com>
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
#	This product includes software developed by Zembu Labs, Inc.
# 4. Neither the name of Zembu Labs nor the names of its employees may
#    be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY ZEMBU LABS, INC. ``AS IS'' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAR-
# RANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DIS-
# CLAIMED.  IN NO EVENT SHALL ZEMBU LABS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# chkconfig - configuration state checker
#
# This script is written to work with the NetBSD (1.5 and later) rc system. 
# It is meant to provide the same functionality as found in IRIX chkconfig. 
# This script has nothing to do with the abortion produced by RedHat that
# has the same name.
#
# chkconfig makes use of the '-k' flag to rcorder. It will not work 
# with versions of rcorder that do not support '-k'.
#
# Dan Mercer <dmercer@zembu.com>

. /etc/rc.subr

display()
{
	# ouput $1 with 'on' or 'off' based on the return of checkyesno()
	# Returns 0 for yes, 1 for no.

	_name=$1	
	load_rc_config ${_name}
	if checkyesno ${_name}; then
		printf "\t%-15s\t\ton\n" ${_name}
		return 0
	else
		printf "\t%-15s\t\toff\n" ${_name}
		return 1
	fi
}

exists()
{
	# Returns true if an executable named $1 exists
	# in /etc/rc.d/

	_name=$1
	fqp="/etc/rc.d/${_name}"
	if [ -x "${fqp}" ]; then
		return 0
	else
		usage "${fqp} does not exist"
		return 1
	fi
}

is_valid()
{
	# Returns true if $1 appears to be a valid NetBSD
	# rc script.

	_name=$1
	fqp="/etc/rc.d/${_name}"
	if ! grep -s '. /etc/rc.subr' ${fqp} > /dev/null 2>&1; then
		usage "${fqp} does not appear to be a NetBSD rc script"
		return 1
	elif ! grep -s '# KEYWORD:' ${fqp} > /dev/null 2>&1; then
		if [ ${force} -ne 1 ]; then
			usage "${fqp} doesn't contain a KEYWORD directive. Use -f"
		else
			return 1
		fi
	else
		is_chkconfig=`grep -s '# KEYWORD:' ${fqp}|grep -s ${KEYWORD}`
		if [ "${is_chkconfig}" ]; then
			return 0
		else
			if [ ${force} -ne 1 ]; then
				usage "${fqp} not under chkconfig control. Use -f"
			else
				return 1
			fi	
		fi
	fi
	return 1
}

add_keyword()
{
	# Adds the 'chkconfig' keyword to $1 if it is not
	# there already, returning a 0. Otherwise exits
	# with an appropriate usage error.

	_name=$1
        fqp="/etc/rc.d/${_name}"
	if is_valid ${_name}; then
		usage "${fqp} is already managed by chkconfig."
	else
		echo '# KEYWORD: chkconfig' >> ${fqp}
		return 0
	fi
}

usage()
{
	# Print a (hopefully) useful usage message and exit nonzero.
	# We don't make use of err() from rc.subr because we
	# don't want error messages going to syslog.

	_err=$1
	echo "Error: ${_err}"
	echo "Usage: $0 flag"
	echo "       $0 flag [ on | off ] "
	echo "       $0 [-f] flag [ on | off ]"
	exit 1
}

on()
{
	_name=$1
	if [ ${force} -eq 1 ]; then
		add_keyword ${_name}
	fi

	if is_valid ${_name}; then
		output="/etc/rc.conf.d/${_name}"
		echo "${_name}=YES" > "${output}"
	fi
	return 0
}

off()
{
	_name=$1
	if [ ${force} -eq 1 ]; then
		add_keyword ${_name}
	fi

	if is_valid ${_name}; then
		output="/etc/rc.conf.d/${_name}"
		echo "${_name}=NO" > "${output}"
	fi
	return 0
}

KEYWORD='chkconfig'
action='show'
force=0

for i
do
	case $1 in
	-f)
		force=1
		;;
	on)
		action='on'
		break
		;;
	off)
		action='off'
		break
		;;
	-*)
		usage "Invalid argument ${i}"
		exit 1
		;;
	*)
		rcfile=${i}
		;;
	esac
	shift
done

case ${action} in
	show)
		if [ ${force} -eq 1 ]; then
			usage "-f flag requires 'on' or 'off'"
		fi
		if [ ! ${rcfile} ]; then
			printf "\tService\t\t\tState\n"
			printf "\t=======\t\t\t=====\n"
			for i in `(cd /etc/rc.d; rcorder -k ${KEYWORD} *)`; do
				display ${i}
			done
		else
			if exists ${rcfile} && is_valid ${rcfile}; then
				display ${rcfile}
				exit $?
			else
				usage "Invalid rcfile: ${rcfile}"
			fi
		fi
		exit 0
		;;
	on)
		if exists ${rcfile}; then
			on ${rcfile}
		fi	
		;;
	off)
		if exists ${rcfile}; then
			off ${rcfile}
		fi
		;;
esac
