#	$NetBSD: Makefile.inc,v 1.10 2002/04/10 15:05:44 lukem Exp $

.include "../../Makefile.inc"

LDSTATIC=	-static		# only static compilation makes sense here

# Where to find kernel images...
.include <bsd.kernobj.mk>
SRCSYSDIR?=	${_SRC_TOP_}/sys
