#	$NetBSD: Makefile,v 1.2 1999/03/26 20:04:50 kim Exp $

.PATH:	${.CURDIR}/../nslookup \
	${.CURDIR}/../man

WARNS=	0
BINDIR=	/usr/bin
PROG=	dig
SRCS=	${PROG}.c subr.c send.c list.c debug.c

CPPFLAGS+= ${INCLUDE}
LDADD=	${LIBRARY}
MAN=	${PROG}.1

.include "../../Makefile.inc"
.include <bsd.prog.mk>
