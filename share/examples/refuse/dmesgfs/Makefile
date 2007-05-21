# $NetBSD: Makefile,v 1.1 2007/05/21 21:29:23 agc Exp $

PROG=dmesgfs
SRCS=dmesgfs.c virtdir.c
LDADD+= -lrefuse
CPPFLAGS+= -g -I${.CURDIR}/../virtdir
LDFLAGS+=-g
MAN=dmesgfs.8
WARNS=4

.PATH: ${.CURDIR}/../virtdir

.include <bsd.prog.mk>
