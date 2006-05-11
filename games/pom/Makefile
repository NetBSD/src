#	$NetBSD: Makefile,v 1.5 2006/05/11 23:16:28 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	pom
MAN=	pom.6
DPADD=	${LIBM}
LDADD=	-lm

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS+=	-Wno-pointer-sign
.endif
