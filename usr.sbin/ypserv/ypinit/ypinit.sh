#!/bin/sh
#
#	$NetBSD: ypinit.sh,v 1.2 1996/08/09 15:09:04 thorpej Exp $
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

if [ $# -eq 1 ]; then
	if [ $1 = "-m" ]; then		# ypinit -m
		DOMAIN=`${DOMAINNAME}`
		SERVERTYPE=MASTER
		ERROR=
	fi
fi

if [ $# -eq 2 ]; then
	if [ $1 = "-m" ]; then		# ypinit -m domainname
		DOMAIN=${2}
		SERVERTYPE=MASTER
		ERROR=
	fi
	if [ $1 = "-s" ]; then		# ypinit -s master_server
		DOMAIN=`${DOMAINNAME}`
		SERVERTYPE=SLAVE
		MASTER=${2}
		ERROR=
	fi
fi

if [ $# -eq 3 ]; then
	if [ $1 = "-s" ]; then		# ypinit -s master_server domainname
		DOMAIN=${3}
		SERVERTYPE=SLAVE
		MASTER=${2}
		ERROR=
	fi
fi

if [ X${ERROR} = X"USAGE" ]; then
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
if [ X${DOMAIN} = "X" ]; then
	cat << \__no_domain 1>&2
The local host's YP domain name has not been set.  Please set it with
the domainname(8) command or pass the domain as an argument to ypinit(8).
__no_domain

	exit 1
fi

# Check if hostname is set, don't accept an empty hostname
HOST=`${HOSTNAME}`
if [ X${HOST} = "X" ]; then
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

echo -n "Server type: ${SERVERTYPE} Domain: ${DOMAIN}"
if [ X${SERVERTYPE} = X"SLAVE" ]; then
	echo -n " Master: ${MASTER}"
fi
echo ""
cat << \__notice1

Installing the YP data base will require that you answer a few questions.
Questions will all be asked at the beginning of the procedure.

__notice1

if [ -d "${YP_DIR}/${DOMAIN}" ]; then
	echo -n "Can we destroy the existing ${YP_DIR}/${DOMAIN} and its contents? [y/n: n]  "
	read KILL

	ERROR=
	case ${KILL} in
	y*|Y*)
		ERROR="DELETE"
		;;

	*)
		ERROR=
		;;
	esac

	if [ X${ERROR} = X"DELETE" ]; then
		if ! rm -rf ${YP_DIR}/${DOMAIN}; then
			echo "Can't clean up old directory ${YP_DIR}/${DOMAIN}." 1>&2
			exit 1
		fi
	else
		echo "OK, please clean it up by hand and start again."
		exit 0
	fi
fi

if ! mkdir "${YP_DIR}/${DOMAIN}"; then
	echo "Can't make new directory ${YP_DIR}/${DOMAIN}." 1>&2
	exit 1
fi

if [ X${SERVERTYPE} = X"MASTER" ]; then
	if [ ! -f ${YP_DIR}/Makefile ];	then
		if [ ! -f ${YP_DIR}/Makefile.main ]; then
			echo "Can't find ${YP_DIR}/Makefile.main. " 1>&2
			exit 1
		fi
		cp ${YP_DIR}/Makefile.main ${YP_DIR}/Makefile
	fi

	SUBDIR=`grep "^SUBDIR=" ${YP_DIR}/Makefile`

	if [ X"${SUBDIR}" = "X" ]; then
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

	echo "Done.  Be sure to run \`make' in /var/yp."
fi

if [ X${SERVERTYPE} = X"SLAVE" ]; then
	echo ""

	for MAP in `${YPWHICH} -d ${DOMAIN} -m | cut -d\  -f1`; do
		echo "Transfering ${MAP}..."
		if ! ${YPXFR} -h ${MASTER} -c -d ${DOMAIN} ${MAP}; then
			echo "Can't transfer map ${MAP}." 1>&2
			exit 1
		fi
	done

	echo ""
	echo "Don't forget to update the \`ypservers' on ${MASTER}."
	exit 0
fi
