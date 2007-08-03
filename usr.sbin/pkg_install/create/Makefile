# $NetBSD: Makefile,v 1.8.14.2 2007/08/03 21:50:28 wiz Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_create
SRCS= main.c perform.c pl.c util.c build.c

DPADD=		${LIBARCHIVE} ${LIBZ} ${LIBBZ2}
LDADD=		-larchive -lz -lbz2

.include <bsd.init.mk>

.PATH:	${DIST}/create

.include <bsd.prog.mk>
