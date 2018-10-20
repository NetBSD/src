# $NetBSD: Makefile,v 1.3.2.1 2018/10/20 06:58:23 pgoyette Exp $

WARNS=	6

PROG=	mount_qemufwcfg
SRCS=	fwcfg.c virtdir.c
DPADD+=	${LIBREFUSE} ${LIBPUFFS} ${LIBUTIL}
LDADD=	-lrefuse -lpuffs -lutil
MAN=	mount_qemufwcfg.8

CPPFLAGS+=	-D_KERNTYPES

.include <bsd.prog.mk>
