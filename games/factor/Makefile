#	$NetBSD: Makefile,v 1.5 1997/05/06 20:45:02 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	factor
SRCS=	factor.c pr_tbl.c
CFLAGS+=-I${.CURDIR}/../primes
MAN=	factor.6
MLINKS+=factor.6 primes.6
.PATH.c: ${.CURDIR}/../primes

.include <bsd.prog.mk>
