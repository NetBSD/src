# $NetBSD: Makefile,v 1.3 2001/12/05 18:34:19 fredette Exp $

all: siop.out osiop.out oosiop.out

PROG=	ncr53cxxx
MKSHARE=no

.include <bsd.prog.mk>

regen: siop.out osiop.out oosiop.out
headers: siop.out osiop.out oosiop.out

siop.out: siop.ss ${PROG}
	./${PROG} siop.ss -p siop.out

osiop.out: osiop.ss ${PROG}
	./${PROG} osiop.ss -p osiop.out

oosiop.out: oosiop.ss ${PROG}
	./${PROG} oosiop.ss -o oosiop.out
