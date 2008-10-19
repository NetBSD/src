# $NetBSD: Makefile,v 1.1.4.2 2008/10/19 22:39:50 haad Exp $

PROG=	bsdcpio
SRCS=	cmdline.c cpio.c err.c matching.c pathmatch.c

.include <bsd.init.mk>

.PATH: ${LIBARCHIVEDIR}/cpio

DPADD=	${LIBARCHIVE} ${LIBBZ2} ${LIBZ}
LDADD=	-larchive -lbz2 -lz

.include <bsd.prog.mk>
