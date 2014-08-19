#	$NetBSD: Makefile,v 1.7.40.1 2014/08/20 00:04:56 tls Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

USE_FORT?=	yes	# cryptographic software

PROG=	bdes

LDADD+=	-lcrypt
DPADD+=	${LIBCRYPT}

.include <bsd.prog.mk>
