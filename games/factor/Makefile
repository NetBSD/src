#	from: @(#)Makefile	5.7 (Berkeley) 4/8/91
#	$Id: Makefile,v 1.3 1994/12/22 09:34:03 cgd Exp $

PROG=	factor
SRCS=	factor.c pr_tbl.c
CFLAGS+=-I${.CURDIR}/../primes
MAN=	factor.6
MLINKS+=factor.6 primes.6
.PATH:	${.CURDIR}/../primes

.include <bsd.prog.mk>
