#	$NetBSD: Makefile,v 1.13 2017/05/21 15:28:41 riastradh Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

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
