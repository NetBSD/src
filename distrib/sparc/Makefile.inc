#	$NetBSD: Makefile.inc,v 1.5 2000/05/03 00:10:34 sjg Exp $

.include "../../Makefile.inc"

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s
VER!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh 
