#	$NetBSD: Makefile,v 1.3 1999/11/20 19:47:11 veego Exp $

.include "../Makefile.inc"
.PATH:	${BIND_DIST_DIR}/bin/dig \
	${BIND_DIST_DIR}/bin/nslookup \
	${BIND_DIST_DIR}/man

BINDIR=	/usr/bin
PROG=	dig
SRCS=	${PROG}.c subr.c send.c list.c debug.c

CPPFLAGS+= ${INCLUDE}
LDADD=	${LIBRARY}
MAN=	${PROG}.1

.include "../../Makefile.inc"
.include <bsd.prog.mk>
