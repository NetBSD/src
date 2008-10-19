# $NetBSD: Makefile,v 1.1.4.2 2008/10/19 22:40:52 haad Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_create
SRCS= main.c perform.c pl.c util.c build.c

DPADD=		${LIBARCHIVE} ${LIBZ} ${LIBBZ2}
LDADD=		-larchive -lz -lbz2

.include <bsd.init.mk>

.PATH:	${DIST}/create

.include <bsd.prog.mk>
