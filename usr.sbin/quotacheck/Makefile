#	$NetBSD: Makefile,v 1.14 1998/03/18 17:22:38 bouyer Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	quotacheck
SRCS=	quotacheck.c fsutil.c preen.c ffs_bswap.c
MAN=	quotacheck.8
DPADD=	${LIBUTIL}
LDADD+=	-lutil
CPPFLAGS+=-I${.CURDIR}/../../sbin/fsck
.PATH:	${.CURDIR}/../../sbin/fsck ../../sys/ufs/ffs

.include <bsd.prog.mk>
