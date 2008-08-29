#	$NetBSD: Makefile,v 1.7 2008/08/29 00:02:22 gmcgarry Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	pom
MAN=	pom.6
DPADD=	${LIBM}
LDADD=	-lm

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.pom.c+=	-Wno-pointer-sign
.endif
