#!/bin/sh
#
#	Shell script to start the dnssec-signer
#	command out of the example directory
#

chroot `pwd` ZKT_CONFFILE=`pwd`/dnssec.conf ../../dnssec-signer "$@"

if test ! -f dnssec.conf
then
	echo Please start this skript out of the flat or hierarchical sub directory
	exit 1
fi
ZKT_CONFFILE=`pwd`/dnssec.conf ../../dnssec-signer "$@"
