#	$NetBSD: Makefile,v 1.6.2.1 1997/11/08 05:57:39 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	factor
SRCS=	factor.c pr_tbl.c
CPPFLAGS+=-I${.CURDIR}/../primes
MAN=	factor.6
MLINKS+=factor.6 primes.6
.PATH:	${.CURDIR}/../primes

.include <bsd.prog.mk>
