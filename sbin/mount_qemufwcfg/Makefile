# $NetBSD: Makefile,v 1.5 2021/12/04 06:42:39 pho Exp $

WARNS=	6

PROG=	mount_qemufwcfg
SRCS=	fwcfg.c virtdir.c
DPADD+=	${LIBREFUSE} ${LIBPUFFS} ${LIBUTIL}
LDADD=	-lrefuse -lpuffs -lutil
MAN=	mount_qemufwcfg.8

.include <bsd.prog.mk>
