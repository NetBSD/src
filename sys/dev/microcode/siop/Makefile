# $NetBSD: Makefile,v 1.4 2001/12/12 01:50:04 tv Exp $

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
