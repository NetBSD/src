#	$NetBSD: Makefile,v 1.3 1999/03/26 20:12:44 kim Exp $

.PATH:	${.CURDIR}/../man

WARNS=	0
BINDIR=	/usr/bin
PROG=	host
SRCS=	${PROG}.c

CPPFLAGS+= ${INCLUDE}
LDADD=	${LIBRARY}
MAN=	${PROG}.1

.include "../../Makefile.inc"
.include <bsd.prog.mk>
