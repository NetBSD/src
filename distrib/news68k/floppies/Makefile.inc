#	$NetBSD: Makefile.inc,v 1.2 2000/05/02 16:06:51 sjg Exp $

.include "../../../Makefile.inc"

# REV is revision without dots, as in "14A"
# VER is revision with dots, as in "1.4A"
REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s
VER!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh 
