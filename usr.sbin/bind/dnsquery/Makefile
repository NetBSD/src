#	$NetBSD: Makefile,v 1.2 1999/03/26 20:04:51 kim Exp $

.PATH:	${.CURDIR}/../man

WARNS=	0
BINDIR=	/usr/bin
PROG=	dnsquery
SRCS=	${PROG}.c
CPPFLAGS+= ${INCLUDE}
LDADD=	${LIBRARY}
MAN=	${PROG}.1

.include "../../Makefile.inc"
.include <bsd.prog.mk>
