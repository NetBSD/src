# $NetBSD: Makefile,v 1.5 2002/04/21 22:52:06 bouyer Exp $

all: esiop.out siop.out osiop.out oosiop.out

PROG=	ncr53cxxx
NOSHARE=# defined

.include <bsd.prog.mk>

regen: esiop.out siop.out osiop.out oosiop.out
headers: esiop.out siop.out osiop.out oosiop.out

esiop.out: esiop.ss ${PROG}
	./${PROG} esiop.ss -p esiop.out

siop.out: siop.ss ${PROG}
	./${PROG} siop.ss -p siop.out

osiop.out: osiop.ss ${PROG}
	./${PROG} osiop.ss -p osiop.out

oosiop.out: oosiop.ss ${PROG}
	./${PROG} oosiop.ss -o oosiop.out
