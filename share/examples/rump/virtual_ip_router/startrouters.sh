#!/bin/sh
#
#	$NetBSD: startrouters.sh,v 1.1 2010/03/29 02:11:14 pooka Exp $
#

dummy=
amp="&"
LIF="10.0.XXX.1"
LBCAST="10.0.XXX.255"
LFILE="/tmp/rumpshm_XXX"
LROUTE="10.0.XXX.2"

RIF="10.0.YYY.2"
RBCAST="10.0.YYY.255"
RFILE="/tmp/rumpshm_YYY"
RROUTE="10.0.YYY.1"

die ()
{

	echo $1
	exit 1
}

[ $# != "1" ] && die "usage: script.sh nrouters"
n=${1}

[ "$n" -lt "1" ] && die "n must be between 1 and 254"
[ "$n" -gt "254" ] && die "n must be between 1 and 254"

pkill a.out
rm /tmp/rumpshm_*
rm -f cmds

i=1
ip=0
while [ ${i} -le ${n} ]
do
	lif=`echo ${LIF} | sed "s/XXX/${ip}/"`
	lbcast=`echo ${LBCAST} | sed "s/XXX/${ip}/"`
	lfile=`echo ${LFILE} | sed "s/XXX/${ip}/"`
	lroute=`echo ${LROUTE} | sed "s/XXX/${ip}/"`

	rif=`echo ${RIF} | sed "s/YYY/${i}/"`
	rbcast=`echo ${RBCAST} | sed "s/YYY/${i}/"`
	rfile=`echo ${RFILE} | sed "s/YYY/${i}/"`
	rroute=`echo ${RROUTE} | sed "s/YYY/${i}/"`

	if [ $i = 1 ]
	then
		lroute="0"
	fi

	if [ $i = $n ]
	then
		rif="10.0.255.1"
		rbcast="10.0.255.255"
		rfile="/tmp/rumpshm_255"
		rroute="0"
	fi

	echo ./a.out ${lif} ${lbcast} ${lfile} ${lroute} ${rif} ${rbcast} ${rfile} ${rroute} >> cmds

	ip=${i}
	i=$((i+1))
done

#echo ./specialpint send tcp 10.0.255.10 >> cmds
