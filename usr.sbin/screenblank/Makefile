#	$NetBSD: Makefile,v 1.9 2004/11/25 20:23:09 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

WARNS=3
.if ${MACHINE} == "pmax" || ${MACHINE} == "sun3" || ${MACHINE} == "sun3x"\
   || ${MACHINE} == "sparc" || ${MACHINE} == "amiga"
CPPFLAGS+=-DHAVE_FBIO
.endif

PROG=	screenblank
MAN=	screenblank.1

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
