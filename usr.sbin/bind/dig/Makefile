#	$NetBSD: Makefile,v 1.2.2.1 1999/12/04 17:10:45 he Exp $

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
