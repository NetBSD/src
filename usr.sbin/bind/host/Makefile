#	$NetBSD: Makefile,v 1.3.2.1 1999/12/04 17:10:53 he Exp $

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
