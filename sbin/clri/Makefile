#	$NetBSD: Makefile,v 1.8 1998/03/18 16:51:31 bouyer Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	clri
MAN=	clri.8
SRCS=	clri.c ffs_bswap.c
DPADD=	${LIBUTIL}
LDADD+=	-lutil
.PATH:  ${.CURDIR}/../../sys/ufs/ffs

.include <bsd.prog.mk>
