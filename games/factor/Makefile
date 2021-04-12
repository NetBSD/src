#	$NetBSD: Makefile,v 1.14 2021/04/12 04:19:32 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>

PRIMES=	${NETBSDSRCDIR}/games/primes

PROG=	factor
SRCS=	factor.c pr_tbl.c
CPPFLAGS+=-I${PRIMES}

CPPFLAGS+=-DHAVE_OPENSSL
LDADD+=	-lcrypto -lcrypt
DPADD+=	${LIBCRYPTO} ${LIBCRYPT}

MAN=	factor.6
.PATH:	${PRIMES}

.include <bsd.prog.mk>
