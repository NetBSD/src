#	$NetBSD: Makefile,v 1.16 1999/03/24 05:50:50 mrg Exp $

PROG =	rpc.rstatd
.PATH: ${.CURDIR}/../../usr.bin/vmstat

CPPFLAGS+=-I${.CURDIR}/../../usr.bin/vmstat
SRCS =	dkstats.c rstatd.c rstat_proc.c
MAN =	rpc.rstatd.8
MLINKS=	rpc.rstatd.8 rstatd.8

DPADD=	${LIBRPCSVC} ${LIBKVM}
LDADD=	-lrpcsvc -lkvm

.include <bsd.prog.mk>

# XXX
.if defined(PMAP_NEW)
CPPFLAGS+=      -DPMAP_NEW
.endif
