#	$NetBSD: Makefile.inc,v 1.1 2001/05/18 00:16:38 fredette Exp $

.include "../../Makefile.inc"

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s

LDSTATIC=	-static		# only static compilation makes sense here

# Where to find kernel images...
.include <bsd.kernobj.mk>
SRCSYSDIR?=	${BSDSRCDIR}/sys
