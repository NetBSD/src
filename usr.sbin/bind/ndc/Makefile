#	$NetBSD: Makefile,v 1.3 1999/11/21 16:26:03 itojun Exp $

.include "../Makefile.inc"
.PATH:	${BIND_DIST_DIR}/bin/ndc \
	${BIND_DIST_DIR}/man

PROG=	ndc
SRCS=	${PROG}.c

CPPFLAGS+= ${INCLUDE} -I${.OBJDIR}/../named
# how to deal with this in a clean manner??
CPPFLAGS+= -I${.CURDIR}/../named/obj
LDADD=	${LIBRARY}
MAN=	${PROG}.8

.include "../../Makefile.inc"
.include <bsd.prog.mk>
