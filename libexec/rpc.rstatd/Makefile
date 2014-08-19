#	$NetBSD: Makefile,v 1.20.48.1 2014/08/20 00:02:23 tls Exp $

.include <bsd.own.mk>

PROG =	rpc.rstatd
.PATH: ${NETBSDSRCDIR}/usr.bin/vmstat

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/vmstat
SRCS =	drvstats.c rstatd.c rstat_proc.c
MAN =	rpc.rstatd.8
MLINKS=	rpc.rstatd.8 rstatd.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

.include <bsd.prog.mk>
