#!/bin/sh
#
#	$NetBSD: ypinit.sh,v 1.4.2.3 1997/11/28 09:42:51 mellon Exp $
#
# ypinit.sh - setup a master or slave YP server
#
# Originally written by Mats O Jansson <moj@stacken.kth.se>
# Modified by Jason R. Thorpe <thorpej@NetBSD.ORG>
# Reworked by Luke Mewburn <lukem@netbsd.org>
#

PATH=/bin:/usr/sbin:/usr/bin:${PATH}
DOMAINNAME=/bin/domainname
HOSTNAME=/bin/hostname
ID=/usr/bin/id
MAKEDBM=/usr/sbin/makedbm
YPWHICH=/usr/bin/ypwhich
YPXFR=/usr/sbin/ypxfr

progname=`basename $0`
yp_dir=/var/yp
error=usage				# assume usage error

umask 077				# protect created directories

if [ `${ID} -u` != 0 ]; then
	echo 1>&2 "$progname: you must be root to run this"
	exit 1
fi

case $# in
1)
	if [ $1 = "-m" ]; then		# ypinit -m
		domain=`${DOMAINNAME}`
		servertype=master
		error=
	fi
	;;

2)
	if [ $1 = "-m" ]; then		# ypinit -m domainname
		domain=${2}
		servertype=master
		error=
	fi
	if [ $1 = "-s" ]; then		# ypinit -s master_server
		domain=`${DOMAINNAME}`
		servertype=slave
		master=${2}
		error=
	fi
	;;

3)
	if [ $1 = "-s" ]; then		# ypinit -s master_server domainname
		domain=${3}
		servertype=slave
		master=${2}
		error=
	fi
	;;
esac

if [ "${error}" = "usage" ]; then
	cat 1>&2 << __usage 
usage: ${progname} -m [domainname]
       ${progname} -s master_server [domainname]

The \`-m' flag builds a master YP server, and the \`-s' flag builds
a slave YP server.  When building a slave YP server, \`master_server'
must be an existing, reachable YP server.
__usage

	exit 1
fi

# Check if domainname is set, don't accept an empty domainname
if [ -z "${domain}" ]; then
	cat << __no_domain 1>&2
$progname: The local host's YP domain name has not been set.
	Please set it with the domainname(1) command or pass the domain as
	an argument to ${progname}.
__no_domain

	exit 1
fi

# Check if hostname is set, don't accept an empty hostname
host=`${HOSTNAME}`
if [ -z "${host}" ]; then
	cat 1>&2 << __no_hostname
$progname: The local host's hostname has not been set.
	Please set it with the hostname(1) command.
__no_hostname

	exit 1
fi

# Check if the YP directory exists.
if [ ! -d ${yp_dir} -o -f ${yp_dir} ]; then
	cat 1>&2 << __no_dir
$progname: The directory ${yp_dir} does not exist.
	Restore it from the distribution.
__no_dir

	exit 1
fi

echo "Server type: ${servertype}"
echo "Domain:      ${domain}"
if [ "${servertype}" != "master" ]; then
	echo "Master:      ${master}"
fi
echo ""
cat << __notice1

Installing the YP database may require that you answer a few questions.
Any configuration questions will be asked at the beginning of the procedure.

__notice1

if [ -d "${yp_dir}/${domain}" ]; then
	echo	"Can we destroy the existing ${yp_dir}/${domain}"
	echo -n	"and its contents? [y/n: n]  "
	read KILL

	error=
	case ${KILL} in
	y*|Y*)
		error="delete"
		;;

	*)
		error=
		;;
	esac

	if [ "${error}" = "delete" ]; then
		rm -rf ${yp_dir}/${domain}
		if [ $? != 0 ]; then
			echo 1>&2 \
		"$progname: Can't clean up old directory ${yp_dir}/${domain}"
			exit 1
		fi
	else
		echo "OK, please clean it up by hand and start again."
		exit 0
	fi
fi

if ! mkdir "${yp_dir}/${domain}"; then
	echo 1>&2 "$progname: Can't make new directory ${yp_dir}/${domain}"
	exit 1
fi

case ${servertype} in
master)
	if [ ! -f ${yp_dir}/Makefile ];	then
		if [ ! -f ${yp_dir}/Makefile.main ]; then
			echo 1>&2 \
			    "$progname: Can't find ${yp_dir}/Makefile.main"
			exit 1
		fi
		cp ${yp_dir}/Makefile.main ${yp_dir}/Makefile
	fi

	subdir=`grep "^SUBDIR=" ${yp_dir}/Makefile`

	if [ -z "${subdir}" ]; then
		echo 1>&2 \
    "$progname: Can't find line starting with 'SUBDIR=' in ${yp_dir}/Makefile"
		exit 1
	fi

	newsubdir="SUBDIR="
	for dir in `echo ${subdir} | cut -c8-255`; do
		if [ "${dir}" != "${domain}" ]; then
			newsubdir="${newsubdir} ${dir}"
		fi
	done
	newsubdir="${newsubdir} ${domain}"

	if [ -f ${yp_dir}/Makefile.tmp ]; then 
		rm ${yp_dir}/Makefile.tmp
	fi

	mv ${yp_dir}/Makefile ${yp_dir}/Makefile.tmp
	sed -e "s/^${subdir}/${newsubdir}/" ${yp_dir}/Makefile.tmp > \
	    ${yp_dir}/Makefile
	rm ${yp_dir}/Makefile.tmp

	if [ ! -f ${yp_dir}/Makefile.yp ]; then
		echo 1>&2 "$progname: Can't find ${yp_dir}/Makefile.yp"
		exit 1
	fi

	cp ${yp_dir}/Makefile.yp ${yp_dir}/${domain}/Makefile

	# Create `ypservers' with own name, so that yppush won't
	# lose when we run "make".
	(
		cd ${yp_dir}/${domain}
		echo "$host $host" > ypservers
		${MAKEDBM} ypservers ypservers
	)

	echo "Done.  Be sure to run \`make' in ${yp_dir}."

	;;

slave)
	echo ""

	maps=`${YPWHICH} -d ${domain} -h ${master} -f -m 2>/dev/null | \
	    awk '{ if (substr($2, 1, length("'$master'")) == "'$master'") \
		print $1; }'`

	if [ -z "${maps}" ]; then
		cat 1>&2 << __no_maps
$progname: Can't find any maps for ${domain} on ${master}
	Please check that the appropriate YP service is running.
__no_maps
		exit 1
	fi

	for map in ${maps}; do
		echo "Transferring ${map}..."
		if ! ${YPXFR} -h ${master} -c -d ${domain} ${map}; then
			echo 1>&2 "$progname: Can't transfer map ${map}"
			exit 1
		fi
	done

	cat << __dont_forget

Don't forget to update the \`ypservers' on ${master},
by adding an entry similar to:
  ${host} ${host}

__dont_forget
	exit 0

	;;

*)
	echo 1>&2 "$progname: unknown servertype \`${servertype}'"
	exit 1
esac
