# $NetBSD: Makefile,v 1.1.6.2 2000/11/20 11:42:06 bouyer Exp $

all: siop.out

PROG=	ncr53cxxx
MKSHARE=no

.include <bsd.prog.mk>

regen: siop.out
headers: siop.out

siop.out: siop.ss ${PROG}
	./${PROG} siop.ss -o siop.out
	
