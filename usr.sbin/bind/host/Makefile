#	$NetBSD: Makefile,v 1.4 1999/11/20 19:47:13 veego Exp $

.include "../Makefile.inc"
.PATH:	${BIND_DIST_DIR}/bin/host \
	${BIND_DIST_DIR}/man

BINDIR=	/usr/bin
PROG=	host
SRCS=	${PROG}.c

CPPFLAGS+= ${INCLUDE}
LDADD=	${LIBRARY}
MAN=	${PROG}.1

.include "../../Makefile.inc"
.include <bsd.prog.mk>
