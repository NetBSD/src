#	$NetBSD: Makefile,v 1.2 1999/11/20 19:47:59 veego Exp $

.include "../Makefile.inc"
.PATH:	${BIND_DIST_DIR}/bin/ndc \
	${BIND_DIST_DIR}/man

PROG=	ndc
SRCS=	${PROG}.c

CPPFLAGS+= ${INCLUDE} -I${.OBJDIR}/../named
LDADD=	${LIBRARY}
MAN=	${PROG}.8

.include "../../Makefile.inc"
.include <bsd.prog.mk>
