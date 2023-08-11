#	$NetBSD: Makefile,v 1.14.6.1 2023/08/11 13:43:41 martin Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>

PRIMES=	${NETBSDSRCDIR}/games/primes

PROG=	factor
SRCS=	factor.c pr_tbl.c
CPPFLAGS+=-I${PRIMES}

CPPFLAGS+=-DHAVE_OPENSSL
LDADD+=	-lcrypto -lcrypt
DPADD+=	${LIBCRYPTO} ${LIBCRYPT}

COPTS.factor.c+= -Wno-error=deprecated-declarations

MAN=	factor.6
.PATH:	${PRIMES}

.include <bsd.prog.mk>
