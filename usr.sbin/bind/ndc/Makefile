#	$NetBSD: Makefile,v 1.6 2001/01/11 13:15:06 enami Exp $

.include "../Makefile.inc"
.PATH:	${BIND_DIST_DIR}/bin/ndc \
	${BIND_DIST_DIR}/man

NAMEDOBJ!=cd ${.CURDIR}/../named && ${PRINTOBJDIR}

PROG=	ndc
SRCS=	${PROG}.c

CPPFLAGS+= ${INCLUDE} -I${NAMEDOBJ}
LDADD=	${LIBRARY}
MAN=	${PROG}.8

.include "../../Makefile.inc"
.include <bsd.prog.mk>
