#	$NetBSD: Makefile,v 1.20 2019/10/13 07:28:11 mrg Exp $

.include <bsd.own.mk>

PROG =	rpc.rusersd
SRCS =	rusersd.c rusers_proc.c utmpentry.c
MAN =	rpc.rusersd.8
MLINKS=	rpc.rusersd.8 rusersd.8

.PATH.c: ${NETBSDSRCDIR}/usr.bin/who

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP


DPADD+=	${LIBRPCSVC}
LDADD+=	-lrpcsvc

COPTS.rusers_proc.c+=	${GCC_NO_STRINGOP_TRUNCATION} ${GCC_NO_STRINGOP_OVERFLOW} ${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
