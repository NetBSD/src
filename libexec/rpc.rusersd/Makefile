#	$NetBSD: Makefile,v 1.21 2023/06/03 09:09:11 lukem Exp $

.include <bsd.own.mk>

PROG =	rpc.rusersd
SRCS =	rusersd.c rusers_proc.c utmpentry.c
MAN =	rpc.rusersd.8
MLINKS=	rpc.rusersd.8 rusersd.8

.PATH.c: ${NETBSDSRCDIR}/usr.bin/who

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP


DPADD+=	${LIBRPCSVC}
LDADD+=	-lrpcsvc

COPTS.rusers_proc.c+=	${CC_WNO_STRINGOP_TRUNCATION} ${CC_WNO_STRINGOP_OVERFLOW} ${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
