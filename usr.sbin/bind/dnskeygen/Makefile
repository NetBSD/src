#	$NetBSD: Makefile,v 1.1 1999/11/20 19:47:12 veego Exp $

.include "../Makefile.inc"
.PATH:	${BIND_DIST_DIR}/bin/dnskeygen \
	${BIND_DIST_DIR}/man

BINDIR=	/usr/libexec
PROG=	dnskeygen
SRCS=	${PROG}.c
CPPFLAGS+= ${INCLUDE}
LDADD=	${LIBRARY}
MAN=	${PROG}.1

.include "../../Makefile.inc"
.include <bsd.prog.mk>
