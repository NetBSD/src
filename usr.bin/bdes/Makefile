#	$NetBSD: Makefile,v 1.1.1.1 2000/06/16 16:50:39 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	bdes

#BINDIR= /usr/bin
LDADD+=	-lcrypt
DPADD+=	${LIBCRYPT}

.include <bsd.prog.mk>
