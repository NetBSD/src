# $NetBSD: Makefile,v 1.1.8.3 2002/06/20 03:45:13 nathanw Exp $

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
