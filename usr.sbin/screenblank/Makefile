#	$NetBSD: Makefile,v 1.3 1997/07/23 20:23:16 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.if ${MACHINE} == "pmax" || ${MACHINE} == "sun3" \
   || ${MACHINE} == "sparc" || ${MACHINE} == "amiga"
PROG=	screenblank

DPADD=	${LIBM}
LDADD=	-lm
.endif

MAN=	screenblank.1

WARNS=1

.include <bsd.prog.mk>
