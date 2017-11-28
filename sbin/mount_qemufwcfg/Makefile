# $NetBSD: Makefile,v 1.3 2017/11/28 11:59:16 wiz Exp $

WARNS=	6

PROG=	mount_qemufwcfg
SRCS=	fwcfg.c virtdir.c
DPADD+=	${LIBREFUSE} ${LIBUTIL}
LDADD=	-lrefuse -lutil
MAN=	mount_qemufwcfg.8

CPPFLAGS+=	-D_KERNTYPES

.include <bsd.prog.mk>
