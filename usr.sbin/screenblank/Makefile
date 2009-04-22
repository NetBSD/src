#	$NetBSD: Makefile,v 1.11 2009/04/22 15:23:08 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.if ${MACHINE} == "pmax" || ${MACHINE} == "sun3" || ${MACHINE} == "sun3x" \
   || ${MACHINE_ARCH} == "sparc" || ${MACHINE} == "amiga"
CPPFLAGS+=-DHAVE_FBIO
.endif

PROG=	screenblank
MAN=	screenblank.1

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
