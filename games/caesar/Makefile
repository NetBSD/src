#	$NetBSD: Makefile,v 1.9 2021/05/01 20:21:25 rillig Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	caesar
MAN=	caesar.6
DPADD=	${LIBM}
LDADD=	-lm
MLINKS=	caesar.6 rot13.6
SCRIPTS=rot13.sh

WARNS=		6
LINTFLAGS+=	-T

.include <bsd.prog.mk>
