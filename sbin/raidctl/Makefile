#	$NetBSD: Makefile,v 1.7 2000/05/23 00:46:53 thorpej Exp $
PROG=	raidctl 
SRCS=	rf_configure.c rf_layout.c raidctl.c
MAN=	raidctl.8

WARNS=2

LOOKHERE = ${.CURDIR}/../../sys/dev/raidframe

CPPFLAGS+= -DRF_UTILITY=1 -I${LOOKHERE}
.PATH:    ${LOOKHERE}

DPADD=  ${LIBUTIL}
LDADD=  -lutil

.include <bsd.prog.mk>
