# $NetBSD: Makefile,v 1.1 2008/09/19 22:01:26 joerg Exp $

PROG=	bsdcpio
SRCS=	cmdline.c cpio.c err.c matching.c pathmatch.c

.include <bsd.init.mk>

.PATH: ${LIBARCHIVEDIR}/cpio

DPADD=	${LIBARCHIVE} ${LIBBZ2} ${LIBZ}
LDADD=	-larchive -lbz2 -lz

.include <bsd.prog.mk>
