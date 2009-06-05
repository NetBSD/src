# $NetBSD: Makefile,v 1.1.12.2 2009/06/05 17:01:46 snj Exp $

PROG=	bsdcpio
SRCS=	cmdline.c cpio.c err.c matching.c pathmatch.c

.include <bsd.init.mk>

.PATH: ${LIBARCHIVEDIR}/cpio

DPADD=	${LIBARCHIVE} ${LIBBZ2} ${LIBZ}
LDADD=	-larchive -lbz2 -lz

.include <bsd.prog.mk>
