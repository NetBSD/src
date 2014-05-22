#	$NetBSD: Makefile,v 1.7.34.1 2014/05/22 11:42:42 yamt Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

USE_FORT?=	yes	# cryptographic software

PROG=	bdes

LDADD+=	-lcrypt
DPADD+=	${LIBCRYPT}

.include <bsd.prog.mk>
