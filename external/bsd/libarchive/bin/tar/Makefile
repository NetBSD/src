# $NetBSD: Makefile,v 1.1.4.2 2008/10/19 22:39:50 haad Exp $

PROG=	bsdtar
SRCS=	bsdtar.c getdate.y matching.c read.c siginfo.c subst.c \
	tree.c util.c write.c

.include <bsd.init.mk>

.PATH:		${LIBARCHIVEDIR}/tar

DPADD=	${LIBARCHIVE} ${LIBBZ2} ${LIBZ}
LDADD=	-larchive -lbz2 -lz

.include <bsd.prog.mk>
