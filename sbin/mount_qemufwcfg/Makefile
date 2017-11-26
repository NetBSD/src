# $NetBSD: Makefile,v 1.2 2017/11/26 03:06:24 christos Exp $

WARNS=	6

PROG=	mount_qemufwcfg
SRCS=	fwcfg.c virtdir.c
DPADD+=	${LIBREFUSE} ${LIBUTIL}
LDADD=	-lrefuse -lutil
NOMAN=	# defined

CPPFLAGS+=	-D_KERNTYPES

.include <bsd.prog.mk>
