#	$NetBSD: Makefile,v 1.8 1998/03/18 17:19:14 bouyer Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	tunefs
MAN=	tunefs.8
SRCS=	tunefs.c ffs_bswap.c
DPADD=  ${LIBUTIL}
LDADD+= -lutil
.PATH:  ${.CURDIR}/../../sys/ufs/ffs

.include <bsd.prog.mk>
