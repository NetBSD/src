#	$NetBSD: Makefile.inc,v 1.4 2000/05/02 06:47:23 sjg Exp $

# pick up BSDSRCDIR if not already set
.include "../../Makefile.inc"

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s
VER!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh 
