#	from: @(#)Makefile	5.7 (Berkeley) 4/8/91
#	$Id: Makefile,v 1.2 1993/08/01 05:45:35 mycroft Exp $

PROG=	factor
SRCS=	factor.c pr_tbl.c
CFLAGS+=-I${.CURDIR}/../primes
MAN6=	factor.0
MLINKS+=factor.6 primes.6
.PATH:	${.CURDIR}/../primes

.include <bsd.prog.mk>
