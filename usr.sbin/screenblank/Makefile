#	$NetBSD: Makefile,v 1.5 1997/10/18 06:42:57 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.if ${MACHINE} == "pmax" || ${MACHINE} == "sun3" || ${MACHINE} == "sun3x"\
   || ${MACHINE} == "sparc" || ${MACHINE} == "amiga"
PROG=	screenblank

DPADD=	${LIBM}
LDADD=	-lm
.endif

MAN=	screenblank.1

.include <bsd.prog.mk>
