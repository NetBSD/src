#	$NetBSD: Makefile,v 1.7 1998/12/18 01:15:43 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.if ${MACHINE} == "pmax" || ${MACHINE} == "sun3" || ${MACHINE} == "sun3x"\
   || ${MACHINE} == "sparc" || ${MACHINE} == "amiga"
CPPFLAGS+=-DHAVE_FBIO
.endif

PROG=	screenblank
MAN=	screenblank.1

.include <bsd.prog.mk>
