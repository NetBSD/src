# $NetBSD: Makefile,v 1.4 2018/10/03 13:34:44 jmcneill Exp $

WARNS=	6

PROG=	mount_qemufwcfg
SRCS=	fwcfg.c virtdir.c
DPADD+=	${LIBREFUSE} ${LIBPUFFS} ${LIBUTIL}
LDADD=	-lrefuse -lpuffs -lutil
MAN=	mount_qemufwcfg.8

CPPFLAGS+=	-D_KERNTYPES

.include <bsd.prog.mk>
