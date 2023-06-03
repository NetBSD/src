#	$NetBSD: Makefile,v 1.24 2023/06/03 09:09:11 lukem Exp $

.include <bsd.own.mk>

PROG =	rpc.rstatd
.PATH: ${NETBSDSRCDIR}/usr.bin/vmstat

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/vmstat -D_KERNTYPES
SRCS =	drvstats.c rstatd.c rstat_proc.c
MAN =	rpc.rstatd.8
MLINKS=	rpc.rstatd.8 rstatd.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

COPTS.rstat_proc.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
