#	$NetBSD: Makefile.inc,v 1.2.4.1 2002/03/25 17:32:52 he Exp $

.include "../../../Makefile.inc"

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s
VER!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh 
VERDEP= ${BSDSRCDIR}/sys/conf/osrelease.sh

