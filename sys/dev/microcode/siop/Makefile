# $NetBSD: Makefile,v 1.1 2000/04/21 17:57:01 bouyer Exp $

all: siop.out

PROG=	ncr53cxxx
MKSHARE=no

.include <bsd.prog.mk>

regen: siop.out
headers: siop.out

siop.out: siop.ss ${PROG}
	./${PROG} siop.ss -o siop.out
	
