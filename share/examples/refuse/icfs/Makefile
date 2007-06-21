# $NetBSD: Makefile,v 1.1.1.1 2007/06/21 22:05:28 agc Exp $

PROG=icfs
SRCS=icfs.c virtdir.c
LDADD+= -lrefuse
CPPFLAGS+= -I${.CURDIR}/../virtdir
MAN=icfs.8
WARNS=4

.PATH: ${.CURDIR}/../virtdir

.include <bsd.prog.mk>
