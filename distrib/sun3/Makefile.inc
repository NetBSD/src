#	$NetBSD: Makefile.inc,v 1.11 2002/04/26 15:50:48 lukem Exp $

.include "../../Makefile.inc"

LDSTATIC=	-static		# only static compilation makes sense here

# Where to find kernel images...
.include <bsd.kernobj.mk>
SRCSYSDIR?=	${NETBSDSRCDIR}/sys
