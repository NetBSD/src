#	$NetBSD: Makefile,v 1.2.6.1 1999/12/27 18:37:23 wrstuden Exp $

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
