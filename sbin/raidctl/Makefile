#	$NetBSD: Makefile,v 1.16.46.1 2020/04/13 08:03:22 martin Exp $

RUMPPRG=raidctl 
SRCS=	rf_configure.c raidctl.c
MAN=	raidctl.8

DPADD=  ${LIBUTIL}
LDADD=  -lutil

COPTS.raidctl.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
