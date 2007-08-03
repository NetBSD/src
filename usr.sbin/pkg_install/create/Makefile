# $NetBSD: Makefile,v 1.7 2007/08/03 14:04:15 joerg Exp $
# Original from FreeBSD, no rcs id.

PROG= pkg_create
SRCS= main.c perform.c pl.c util.c build.o

DPADD=		${LIBARCHIVE} ${LIBZ} ${LIBBZ2}
LDADD=		-larchive -lz -lbz2

.include <bsd.init.mk>

.PATH:	${DIST}/create

.include <bsd.prog.mk>
