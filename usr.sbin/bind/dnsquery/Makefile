#	$NetBSD: Makefile,v 1.1.1.1 1998/10/05 18:01:59 tron Exp $

.PATH:	${.CURDIR}/../man

WARNS=	0
PROG=	dnsquery
SRCS=	${PROG}.c
CPPFLAGS+= ${INCLUDE}
LDADD=	${LIBRARY}
MAN=	${PROG}.1

.include "../../Makefile.inc"
.include <bsd.prog.mk>
