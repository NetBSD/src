#	$NetBSD: Makefile,v 1.2 1997/06/23 04:51:31 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.if ${MACHINE} == "pmax" || ${MACHINE} == "sun3" \
   || ${MACHINE} == "sparc" || ${MACHINE} == "amiga"
PROG=	screenblank

DPADD=	${LIBM}
LDADD=	-lm
.endif

MAN=	screenblank.1

.include <bsd.prog.mk>
