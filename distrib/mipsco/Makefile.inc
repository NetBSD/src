#	$NetBSD: Makefile.inc,v 1.2 2002/04/10 15:05:42 lukem Exp $

.include "../../Makefile.inc"
.include <bsd.own.mk>

# REV is revision without dots, as in "15R"
# VER is revision with dots, as in "1.5R"
REV!=sh ${_SRC_TOP_}/sys/conf/osrelease.sh -s
VER!=sh ${_SRC_TOP_}/sys/conf/osrelease.sh 
