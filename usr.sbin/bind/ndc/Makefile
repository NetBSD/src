#	$NetBSD: Makefile,v 1.5.4.1 2002/07/01 17:17:25 he Exp $

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
