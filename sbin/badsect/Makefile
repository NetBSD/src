#	$NetBSD: Makefile,v 1.11 1998/03/18 16:50:12 bouyer Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	badsect
SRCS=	badsect.c ffs_bswap.c
MAN=	badsect.8
DPADD=	${LIBUTIL}
LDADD+=	-lutil
.PATH:  ${.CURDIR}/../../sys/ufs/ffs

.include <bsd.prog.mk>
