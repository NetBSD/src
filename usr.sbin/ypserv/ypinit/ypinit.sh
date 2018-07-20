#!/bin/sh
#
#	$NetBSD: ypinit.sh,v 1.13 2018/07/20 13:11:01 kre Exp $
#
# ypinit.sh - setup a master or slave YP server
#
# Originally written by Mats O Jansson <moj@stacken.kth.se>
# Modified by Jason R. Thorpe <thorpej@NetBSD.org>
# Reworked by Luke Mewburn <lukem@NetBSD.org>
#

PATH=/bin:/usr/sbin:/usr/bin:${PATH}
BIN_DOMAINNAME=/bin/domainname
BIN_HOSTNAME=/bin/hostname
ID=/usr/bin/id
INSTALL=/usr/bin/install
MAKEDBM=/usr/sbin/makedbm
YPWHICH=/usr/bin/ypwhich
YPXFR=/usr/sbin/ypxfr

progname=$( basename $0 )
yp_dir=/var/yp
tmpfile=$(mktemp /tmp/ypservers.XXXXXX) || exit 1
trap "rm -f ${tmpfile} ; exit 0" EXIT INT QUIT

umask 077				# protect created directories

if [ $( ${ID} -u ) != 0 ]; then
	echo 1>&2 "$progname: you must be root to run this"
	exit 1
fi

args=$(getopt cl:ms: $*)		# XXX should switch to getopts
if [ $? -eq 0 ]; then
	set -- $args
	for i; do
		case $i in
		"-c")
			servertype=client
			shift
			;;
		"-m")
			servertype=master
			shift
			;;
		"-s")
			servertype=slave
			master=${2}
			shift
			shift
			;;
		"-l")
			noninteractive=yes
			serverlist=${2}
			shift
			shift
			;;
		"--")
			shift
			break
			;;
		esac
	done

	if [ $# -eq 1 ]; then
		domain=${1}
		shift;
	else
		domain=$( ${BIN_DOMAINNAME} )
	fi
fi

if [ -z ${servertype} ]; then
	cat 1>&2 << __usage
usage: 	${progname} -c [domainname] [-l server1,...,serverN]
	${progname} -m [domainname] [-l server1,...,serverN]
	${progname} -s master_server [domainname] [-l server1,...,serverN]

The \`-c' flag sets up a YP client, the \`-m' flag builds a master YP
server, and the \`-s' flag builds a slave YP server.  When building a
slave YP server, \`master_server' must be an existing, reachable YP server.
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
host=$( ${BIN_HOSTNAME} )
if [ -z "${host}" ]; then
	cat 1>&2 << __no_hostname
$progname: The local host's hostname has not been set.
	Please set it with the hostname(1) command.
__no_hostname

	exit 1
fi
if [ "${servertype}" = "slave" -a "${host}" = "${master}" ]; then
	echo 1>&2 \
	    "$progname: cannot setup a YP slave server off the local host."
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
if [ "${servertype}" = "slave" ]; then
	echo "Master:      ${master}"
fi
echo ""

binding_dir=${yp_dir}/binding
if [ ! -d ${binding_dir} ]; then
	cat 1>&2 << __no_dir
$progname: The directory ${binding_dir} does not exist.
	Restore it from the distribution.
__no_dir
	exit 1
fi

if [ -z "${noninteractive}" ]; then
	cat << __client_setup
A YP client needs a list of YP servers to bind to.
Whilst ypbind supports -broadcast, its use is not recommended.
__client_setup

	done=
	while [ -z "${done}" ]; do
		> ${tmpfile}
		cat <<__list_of_servers

Please enter a list of YP servers, in order of preference.
When finished, press RETURN on a blank line or enter EOF.

__list_of_servers

		if [ "${servertype}" != "client" ]; then
			echo ${host} >> ${tmpfile}
			echo "	next host: ${host}";
		fi
		echo -n "	next host: ";

		while read nextserver ; test -n "${nextserver}"
		do
			echo ${nextserver} >> ${tmpfile}
			echo -n "	next host: ";
		done

		if [ -s ${tmpfile} ]; then
			echo ""
			echo "The current servers are:"
			echo ""
			cat ${tmpfile}
			echo ""
			echo -n "Is this correct? [y/n: n] "
			read DONE
			case ${DONE} in
			y*|Y*)
				done=yes
				;;
			esac
		else
			echo    ""
			echo    "You have not supplied any servers."
		fi
		if [ -z "${done}" ]; then
			echo -n "Do you wish to abort? [y/n: n] "
			read ABORT
			case ${ABORT} in
			y*|Y*)
				exit 0
				;;
			esac
		fi
	done
else # interacive
	if [ "${servertype}" != "client" ]; then
		echo ${host} >> ${tmpfile}
	fi
	echo "${serverlist}" | sed -e 's/,/\
/g' >> ${tmpfile}
#the above newline is required
	echo ""
	echo "The current servers are:"
	echo ""
	cat ${tmpfile}
	echo ""
fi # interactive

if [ -s ${tmpfile} ]; then
	${INSTALL} -c -m 0444 ${tmpfile} ${binding_dir}/${domain}.ypservers
fi

if [ "${servertype}" = "client" ]; then
	exit 0
fi

cat << __notice1

Installing the YP database may require that you answer a few questions.
Any configuration questions will be asked at the beginning of the procedure.

__notice1

if [ -d "${yp_dir}/${domain}" ]; then
	echo	"Can we destroy the existing ${yp_dir}/${domain}"
	echo -n	"and its contents? [y/n: n]  "
	read KILL

	case ${KILL} in
	y*|Y*)
		rm -rf ${yp_dir}/${domain}
		if [ $? != 0 ]; then
			echo 1>&2 \
		"$progname: Can't clean up old directory ${yp_dir}/${domain}"
			exit 1
		fi
		;;

	*)
		echo "OK, please clean it up by hand and start again."
		exit 0
		;;
	esac
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

	subdir=$(grep "^SUBDIR=" ${yp_dir}/Makefile)

	if [ -z "${subdir}" ]; then
		echo 1>&2 \
    "$progname: Can't find line starting with 'SUBDIR=' in ${yp_dir}/Makefile"
		exit 1
	fi

	newsubdir="SUBDIR="
	for dir in $(echo ${subdir} | cut -c8-255); do
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

	maps=$( ${YPWHICH} -d ${domain} -h ${master} -f -m 2>/dev/null |
	    awk '{ if (substr($2, 1, length("'$master'")) == "'$master'") \
		print $1; }' )

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
