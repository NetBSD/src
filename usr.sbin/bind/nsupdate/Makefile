#	$NetBSD: Makefile,v 1.3.2.1 1999/12/04 17:11:22 he Exp $

.include "../Makefile.inc"
.PATH:	${BIND_DIST_DIR}/bin/nsupdate \
	${BIND_DIST_DIR}/man

BINDIR=	/usr/bin
PROG=	nsupdate
SRCS=	${PROG}.c
CPPFLAGS+= ${INCLUDE}
LDADD=	${LIBRARY}
MAN=	${PROG}.8

.include "../../Makefile.inc"
.include <bsd.prog.mk>
