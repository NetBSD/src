#	$NetBSD: Makefile,v 1.2.2.1 1999/12/04 17:10:49 he Exp $

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
