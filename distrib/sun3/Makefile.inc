#	$NetBSD: Makefile.inc,v 1.8 2000/05/02 16:06:55 sjg Exp $

.include "../../Makefile.inc"

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s

LDSTATIC=	-static		# only static compilation makes sense here

# Where to find kernel images...
.include <bsd.kernobj.mk>
SRCSYSDIR?=	${BSDSRCDIR}/sys
