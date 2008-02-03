# $NetBSD: Makefile,v 1.6 2008/02/03 21:27:27 joerg Exp $
# Original from FreeBSD, no rcs id

PROG= pkg_info
SRCS= main.c perform.c show.c

DPADD=		${LIBARCHIVE} ${LIBZ} ${LIBBZ2}
LDADD=		-larchive -lz -lbz2

.include <bsd.init.mk>

.PATH:	${DIST}/info

.include <bsd.prog.mk>
