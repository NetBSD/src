#	$NetBSD: Makefile,v 1.17 2019/10/13 07:28:12 mrg Exp $

RUMPPRG=raidctl 
SRCS=	rf_configure.c raidctl.c
MAN=	raidctl.8

DPADD=  ${LIBUTIL}
LDADD=  -lutil

COPTS.raidctl.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
