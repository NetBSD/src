#	$NetBSD: Makefile,v 1.7 1997/10/22 04:43:26 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	factor
SRCS=	factor.c pr_tbl.c
CPPFLAGS+=-I${.CURDIR}/../primes
MAN=	factor.6
MLINKS+=factor.6 primes.6
.PATH:	${.CURDIR}/../primes

.include <bsd.prog.mk>
