#	$NetBSD: Makefile,v 1.3 1999/11/20 19:47:13 veego Exp $

.include "../Makefile.inc"
.PATH:	${BIND_DIST_DIR}/bin/dnsquery \
	${BIND_DIST_DIR}/man

BINDIR=	/usr/bin
PROG=	dnsquery
SRCS=	${PROG}.c
CPPFLAGS+= ${INCLUDE}
LDADD=	${LIBRARY}
MAN=	${PROG}.1

.include "../../Makefile.inc"
.include <bsd.prog.mk>
