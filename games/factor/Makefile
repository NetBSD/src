#	$NetBSD: Makefile,v 1.10 2002/09/19 03:15:39 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

# For MKCRYPTO
.include <bsd.own.mk>

PRIMES=	${NETBSDSRCDIR}/games/primes

PROG=	factor
SRCS=	factor.c pr_tbl.c
CPPFLAGS+=-I${PRIMES}

.if (${MKCRYPTO} != "no")
CPPFLAGS+=-DHAVE_OPENSSL
LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
.endif

MAN=	factor.6
MLINKS+=factor.6 primes.6
.PATH:	${PRIMES}

.include <bsd.prog.mk>
