#	$NetBSD: Makefile.inc,v 1.1 2000/09/18 07:21:01 msaitoh Exp $

.include "../../../Makefile.inc"

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s
VER!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh 
