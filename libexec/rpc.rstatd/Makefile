#	$NetBSD: Makefile,v 1.19 2002/09/18 06:24:35 lukem Exp $

.include <bsd.own.mk>

PROG =	rpc.rstatd
.PATH: ${NETBSDSRCDIR}/usr.bin/vmstat

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/vmstat
SRCS =	dkstats.c rstatd.c rstat_proc.c
MAN =	rpc.rstatd.8
MLINKS=	rpc.rstatd.8 rstatd.8

DPADD=	${LIBRPCSVC} ${LIBKVM}
LDADD=	-lrpcsvc -lkvm

.include <bsd.prog.mk>
