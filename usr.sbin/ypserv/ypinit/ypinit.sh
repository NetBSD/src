#!/bin/sh
#
#	$NetBSD: ypinit.sh,v 1.1.1.1 1996/08/09 10:14:59 thorpej Exp $
#
# ypinit.sh - setup a master or slave YP server
#
# Originally written by Mats O Jansson <moj@stacken.kth.se>
# Modified by Jason R. Thorpe <thorpej@NetBSD.ORG>
#

DOMAINNAME=/bin/domainname
HOSTNAME=/bin/hostname
YPWHICH=/usr/bin/ypwhich
YPXFR=/usr/sbin/ypxfr
MAKEDBM=/usr/sbin/makedbm
YP_DIR=/var/yp

ERROR=USAGE				# assume usage error

if [ $# -eq 1 ]
then
	if [ $1 = "-m" ]		# ypinit -m
	then
		DOMAIN=`${DOMAINNAME}`
		SERVERTYPE=MASTER
		ERROR=
	fi
fi

if [ $# -eq 2 ]
then
	if [ $1 = "-m" ]		# ypinit -m domainname
	then
		DOMAIN=${2}
		SERVERTYPE=MASTER
		ERROR=
	fi
	if [ $1 = "-s" ]		# ypinit -s master_server
	then
		DOMAIN=`${DOMAINNAME}`
		SERVERTYPE=SLAVE
		MASTER=${2}
		ERROR=
	fi
fi

if [ $# -eq 3 ]
then
	if [ $1 = "-s" ]		# ypinit -s master_server domainname
	then
		DOMAIN=${3}
		SERVERTYPE=SLAVE
		MASTER=${2}
		ERROR=
	fi
fi

if [ "${ERROR}" = "USAGE" ]
then
	cat << \__usage 1>&2
usage: ypinit -m [domainname]
       ypinit -s master_server [domainname]

The `-m' flag builds a master YP server, and the `-s' flag builds
a slave YP server.  When building a slave YP server, `master_server'
must be an existing, reachable YP server.
__usage

	exit 1
fi

# Check if domainname is set, don't accept an empty domainname
if [ -z "${DOMAIN}" ]
then
	cat << \__no_domain 1>&2
The local host's YP domain name has not been set.  Please set it with
the domainname(8) command or pass the domain as an argument to ypinit(8).
__no_domain

	exit 1
fi

# Check if hostname is set, don't accept an empty hostname
HOST=`${HOSTNAME}`
if [ -z "${HOST}" ]
then
	cat << \__no_hostname 1>&2
The local host's hostname has not been set.  Please set it with the
hostname(8) command.
__no_hostname

	exit 1
fi

# Check if the YP directory exists.
if [ ! -d ${YP_DIR} -o -f ${YP_DIR} ]
then
	echo "The directory ${YP_DIR} does not exist.  Restore it from the distribution." 1>&2
	exit 1
fi

echo "Server type: ${SERVERTYPE} Domain: ${DOMAIN} Master: ${MASTER}"
cat << \__notice1

Installing the YP data base will require that you answer a few questions.
Questions will all be asked at the beginning of the procedure.

__notice1

if [ -d ${YP_DIR}/${DOMAIN} ]; then
	echo -n "Can we destroy the existing ${YP_DIR}/${DOMAIN} and its contents? [y/n: n]  "
	read KILL

	ERROR=
	case ${KILL} in
	y*)
		ERROR = DELETE
		;;

	Y*)
		ERROR = DELETE
		;;

	*)
		ERROR=
		;;
	esac

	if [ -z "${ERROR}" ]
	then
		echo "OK, please clean it up by hand and start again."
		exit 0
	fi

	if [ "${ERROR}" = "DELETE" ]
	then
		rm -rf ${YP_DIR}/${DOMAIN}

		if [ $? -ne 0 ]
		then
			echo "Can't clean up old directory ${YP_DIR}/${DOMAIN}." 1>&2
			exit 1
		fi
	fi
fi

mkdir ${YP_DIR}/${DOMAIN}

if [ $? -ne 0 ]
then
	echo "Can't make new directory ${YP_DIR}/${DOMAIN}." 1>&2
	exit 1
fi

if [ "${SERVERTYPE}" = "MASTER" ]; then
	if [ ! -f ${YP_DIR}/Makefile ];	then
		if [ ! -f ${YP_DIR}/Makefile.main ]; then
			echo "Can't find ${YP_DIR}/Makefile.main. " 1>&2
			exit 1
		fi
		cp ${YP_DIR}/Makefile.main ${YP_DIR}/Makefile
	fi

	SUBDIR=`grep "^SUBDIR=" ${YP_DIR}/Makefile`

	if [ -z "${SUBDIR}" ]; then
		echo "Can't find line starting with 'SUBDIR=' in ${YP_DIR}/Makefile. " 1>&2
		exit 1
	fi

	NEWSUBDIR="SUBDIR="
	for DIR in `echo ${SUBDIR} | cut -c8-255`; do
		if [ ${DIR} != ${DOMAIN} ]; then
			NEWSUBDIR="${NEWSUBDIR} ${DIR}"
		fi
	done
	NEWSUBDIR="${NEWSUBDIR} ${DOMAIN}"

	if [ -f ${YP_DIR}/Makefile.tmp ]; then 
		rm ${YP_DIR}/Makefile.tmp
	fi

	mv ${YP_DIR}/Makefile ${YP_DIR}/Makefile.tmp
	sed -e "s/^${SUBDIR}/${NEWSUBDIR}/" ${YP_DIR}/Makefile.tmp > \
	    ${YP_DIR}/Makefile
	rm ${YP_DIR}/Makefile.tmp

	if [ ! -f ${YP_DIR}/Makefile.yp ]; then
		echo "Can't find ${YP_DIR}/Makefile.yp." 1>&2
		exit 1
	fi

	cp ${YP_DIR}/Makefile.yp ${YP_DIR}/${DOMAIN}/Makefile

	# Create an empty `ypservers' map so that yppush won't
	# lose when we run "make".
	(
		cd ${YP_DIR}/${DOMAIN}
		touch ypservers
		cat ypservers | ${MAKEDBM} - ypservers
	)
fi

if [ "${SERVERTYPE}" = "SLAVE" ]; then
	echo ""

	for MAP in `${YPWHICH} -d ${DOMAIN} -m | cut -d\  -f1`; do
		echo "Transfering ${MAP}..."
		${YPXFR} -h ${MASTER} -c -d ${DOMAIN} ${MAP}

		if [ $?  -ne 0 ]; then
			echo "Can't transfer map ${MAP}." 1>&2
			exit 1
		fi
	done

	echo ""
	echo "Don't forget to update the `ypservers' on ${MASTER}."
	exit 0
fi
