#	$NetBSD: Makefile,v 1.1.1.1 1998/10/05 18:01:59 tron Exp $

.PATH:	${.CURDIR}/../nslookup \
	${.CURDIR}/../man

WARNS=	0
PROG=	dig
SRCS=	${PROG}.c subr.c send.c list.c debug.c

CPPFLAGS+= ${INCLUDE}
LDADD=	${LIBRARY}
MAN=	${PROG}.1

.include "../../Makefile.inc"
.include <bsd.prog.mk>
