# $NetBSD: Makefile,v 1.3.4.1 2019/06/10 22:05:34 christos Exp $

WARNS=	6

PROG=	mount_qemufwcfg
SRCS=	fwcfg.c virtdir.c
DPADD+=	${LIBREFUSE} ${LIBPUFFS} ${LIBUTIL}
LDADD=	-lrefuse -lpuffs -lutil
MAN=	mount_qemufwcfg.8

CPPFLAGS+=	-D_KERNTYPES

.include <bsd.prog.mk>
