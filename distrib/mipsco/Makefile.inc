#	$NetBSD: Makefile.inc,v 1.1 2001/02/19 07:45:38 wdk Exp $

.include "../../Makefile.inc"
.include <bsd.own.mk>

# REV is revision without dots, as in "15R"
# VER is revision with dots, as in "1.5R"
REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s
VER!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh 
