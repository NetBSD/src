#	$NetBSD: Makefile,v 1.19 2002/03/28 09:58:10 jmc Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	iostat
MAN=	iostat.8

.PATH:	${.CURDIR}/../../usr.bin/vmstat

CPPFLAGS+=-I${.CURDIR}/../../usr.bin/vmstat

# dkstats.c pulled in from ../../usr.bin/vmstat
SRCS=	dkstats.c iostat.c

DPADD=	${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>
