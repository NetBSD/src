#	$NetBSD: Makefile.inc,v 1.7 2002/01/11 17:42:57 pk Exp $

.include "../../Makefile.inc"
.include <bsd.own.mk>

# REV is revision without dots, as in "12C"
# VER is revision with dots, as in "1.2C"
REV!=sh ${KERNSRCDIR}/conf/osrelease.sh -s
VER!=sh ${KERNSRCDIR}/conf/osrelease.sh 
