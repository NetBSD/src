#	$NetBSD: Makefile.inc,v 1.8.4.1 2002/03/25 17:33:04 he Exp $

.include "../../Makefile.inc"

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s
VERDEP= ${BSDSRCDIR}/sys/conf/osrelease.sh

LDSTATIC=	-static		# only static compilation makes sense here

# Where to find kernel images...
.include <bsd.kernobj.mk>
SRCSYSDIR?=	${BSDSRCDIR}/sys
