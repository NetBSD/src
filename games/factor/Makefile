#	$NetBSD: Makefile,v 1.8 2002/06/15 02:12:23 simonb Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	factor
SRCS=	factor.c pr_tbl.c
CPPFLAGS+=-I${.CURDIR}/../primes
LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
MAN=	factor.6
MLINKS+=factor.6 primes.6
.PATH:	${.CURDIR}/../primes

.include <bsd.prog.mk>
