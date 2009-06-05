# $NetBSD: Makefile,v 1.1.14.2 2009/06/05 17:19:29 snj Exp $

PROG=	bsdtar
SRCS=	bsdtar.c getdate.y matching.c read.c siginfo.c subst.c \
	tree.c util.c write.c

.include <bsd.init.mk>

.PATH:		${LIBARCHIVEDIR}/tar

DPADD=	${LIBARCHIVE} ${LIBBZ2} ${LIBZ}
LDADD=	-larchive -lbz2 -lz

.include <bsd.prog.mk>
