#	$NetBSD: Makefile,v 1.9 2002/06/16 22:24:01 itojun Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

# For MKCRYPTO
.include <bsd.own.mk>

PROG=	factor
SRCS=	factor.c pr_tbl.c
CPPFLAGS+=-I${.CURDIR}/../primes

.if (${MKCRYPTO} != "no")
CPPFLAGS+=-DHAVE_OPENSSL
LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
.endif

MAN=	factor.6
MLINKS+=factor.6 primes.6
.PATH:	${.CURDIR}/../primes

.include <bsd.prog.mk>
