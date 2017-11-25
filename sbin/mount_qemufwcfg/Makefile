# $NetBSD: Makefile,v 1.1 2017/11/25 23:23:39 jmcneill Exp $

PROG=	mount_qemufwcfg
SRCS=	fwcfg.c virtdir.c
DPADD+=	${LIBREFUSE}
LDADD=	-lrefuse
NOMAN=	# defined
WARNS=	3

CPPFLAGS+=	-D_KERNTYPES

.include <bsd.prog.mk>
