#	$NetBSD: Makefile,v 1.16 2025/07/19 11:01:58 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>

PRIMES=	${NETBSDSRCDIR}/games/primes

PROG=	factor
SRCS=	factor.c pr_tbl.c
CPPFLAGS+=-I${PRIMES}

CPPFLAGS+=-DHAVE_OPENSSL
LDADD+=	-lcrypto -lcrypt -lpthread
DPADD+=	${LIBCRYPTO} ${LIBCRYPT} ${LIBPTHREAD}

COPTS.factor.c+= -Wno-error=deprecated-declarations

MAN=	factor.6
.PATH:	${PRIMES}

.include <bsd.prog.mk>
