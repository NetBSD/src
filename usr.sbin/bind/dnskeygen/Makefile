#	$NetBSD: Makefile,v 1.1.4.1 1999/12/27 18:37:22 wrstuden Exp $

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
