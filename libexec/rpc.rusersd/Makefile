#	$NetBSD: Makefile,v 1.19 2018/02/28 23:45:56 snj Exp $

.include <bsd.own.mk>

PROG =	rpc.rusersd
SRCS =	rusersd.c rusers_proc.c utmpentry.c
MAN =	rpc.rusersd.8
MLINKS=	rpc.rusersd.8 rusersd.8

.PATH.c: ${NETBSDSRCDIR}/usr.bin/who

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP


DPADD+=	${LIBRPCSVC}
LDADD+=	-lrpcsvc

.include <bsd.prog.mk>

