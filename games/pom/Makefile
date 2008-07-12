#	$NetBSD: Makefile,v 1.6 2008/07/12 12:31:20 gmcgarry Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	pom
MAN=	pom.6
DPADD=	${LIBM}
LDADD=	-lm

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4 || defined(HAVE_PCC)
COPTS.pom.c+=	-Wno-pointer-sign
.endif
