#	$NetBSD: Makefile,v 1.1.1.1.6.1 1999/12/27 18:37:33 wrstuden Exp $

.include "../Makefile.inc"
.PATH:	${BIND_DIST_DIR}/bin/ndc \
	${BIND_DIST_DIR}/man

NAMEDOBJ!=cd ${.CURDIR}/../named; ${MAKE} print-objdir

PROG=	ndc
SRCS=	${PROG}.c

CPPFLAGS+= ${INCLUDE} -I${NAMEDOBJ}
LDADD=	${LIBRARY}
MAN=	${PROG}.8

.include "../../Makefile.inc"
.include <bsd.prog.mk>
