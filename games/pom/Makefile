#	$NetBSD: Makefile,v 1.5.20.1 2008/09/18 04:39:58 wrstuden Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	pom
MAN=	pom.6
DPADD=	${LIBM}
LDADD=	-lm

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.pom.c+=	-Wno-pointer-sign
.endif
