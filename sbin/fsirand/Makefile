#	$NetBSD: Makefile,v 1.2 1998/03/18 17:07:14 bouyer Exp $

PROG=	fsirand
SRCS=	fsirand.c ffs_bswap.c
MAN=	fsirand.8
DPADD=	${LIBUTIL}
LDADD+=	-lutil

.PATH:  ${.CURDIR}/../../sys/ufs/ffs

.include <bsd.prog.mk>
