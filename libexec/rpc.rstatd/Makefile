#	$NetBSD: Makefile,v 1.13 1997/10/22 05:55:52 lukem Exp $

PROG =	rpc.rstatd
.PATH: ${.CURDIR}/../../usr.bin/vmstat

CPPFLAGS+=-I${.CURDIR}/../../usr.bin/vmstat
SRCS =	dkstats.c rstatd.c rstat_proc.c
MAN =	rpc.rstatd.8
MLINKS=	rpc.rstatd.8 rstatd.8

DPADD=	${LIBRPCSVC} ${LIBKVM}
LDADD=	-lrpcsvc -lkvm

.include <bsd.prog.mk>
