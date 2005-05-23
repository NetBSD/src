#	$NetBSD: Makefile,v 1.7 2005/05/23 23:08:06 rillig Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	caesar
MAN=	caesar.6
DPADD=	${LIBM}
LDADD=	-lm
MLINKS=	caesar.6 rot13.6
SCRIPTS=rot13.sh
WARNS=	4

.include <bsd.prog.mk>
