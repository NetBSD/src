# $NetBSD: Makefile,v 1.1.8.1 2001/06/21 20:04:13 nathanw Exp $

all: siop.out osiop.out

PROG=	ncr53cxxx
MKSHARE=no

.include <bsd.prog.mk>

regen: siop.out osiop.out
headers: siop.out osiop.out

siop.out: siop.ss ${PROG}
	./${PROG} siop.ss -o siop.out

osiop.out: osiop.ss ${PROG}
	./${PROG} osiop.ss -p osiop.out
