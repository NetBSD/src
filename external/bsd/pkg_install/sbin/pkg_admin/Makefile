# $NetBSD: Makefile,v 1.1 2008/09/30 19:19:56 joerg Exp $

PROG= pkg_admin
SRCS= audit.c check.c config.c main.c

LINK_PKGVUL=	yes

.include <bsd.init.mk>

.PATH:	${DIST}/admin
.include <bsd.prog.mk>
