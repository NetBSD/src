#	$NetBSD: Makefile,v 1.12 2009/12/13 05:01:33 nakayama Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.if ${MACHINE} == "pmax" || ${MACHINE} == "sun3" || ${MACHINE} == "sun3x" \
   || ${MACHINE} == "sparc" || ${MACHINE} == "amiga"
CPPFLAGS+=-DHAVE_FBIO
.endif

PROG=	screenblank
MAN=	screenblank.1

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
