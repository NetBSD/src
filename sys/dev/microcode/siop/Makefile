# $NetBSD: Makefile,v 1.2.2.1 2002/01/10 19:56:11 thorpej Exp $

all: siop.out osiop.out oosiop.out

PROG=	ncr53cxxx
NOSHARE=# defined

.include <bsd.prog.mk>

regen: siop.out osiop.out oosiop.out
headers: siop.out osiop.out oosiop.out

siop.out: siop.ss ${PROG}
	./${PROG} siop.ss -p siop.out

osiop.out: osiop.ss ${PROG}
	./${PROG} osiop.ss -p osiop.out

oosiop.out: oosiop.ss ${PROG}
	./${PROG} oosiop.ss -o oosiop.out
