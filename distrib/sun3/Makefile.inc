#	$NetBSD: Makefile.inc,v 1.9 2002/03/06 12:15:19 lukem Exp $

.include "../../Makefile.inc"

LDSTATIC=	-static		# only static compilation makes sense here

# Where to find kernel images...
.include <bsd.kernobj.mk>
SRCSYSDIR?=	${BSDSRCDIR}/sys
