# $NetBSD: Makefile,v 1.2 2001/04/30 04:47:51 tsutsui Exp $

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
