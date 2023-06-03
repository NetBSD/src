#	$NetBSD: Makefile,v 1.18 2023/06/03 09:09:13 lukem Exp $

RUMPPRG=raidctl 
SRCS=	rf_configure.c raidctl.c
MAN=	raidctl.8

DPADD=  ${LIBUTIL}
LDADD=  -lutil

COPTS.raidctl.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
