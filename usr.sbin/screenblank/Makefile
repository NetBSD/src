#	$NetBSD: Makefile,v 1.4 1997/09/24 07:22:40 jeremy Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.if ${MACHINE} == "pmax" || ${MACHINE} == "sun3" || ${MACHINE} == "sun3x"\
   || ${MACHINE} == "sparc" || ${MACHINE} == "amiga"
PROG=	screenblank

DPADD=	${LIBM}
LDADD=	-lm
.endif

MAN=	screenblank.1

WARNS=1

.include <bsd.prog.mk>
