#	$NetBSD: Makefile,v 1.18 2009/04/22 15:23:07 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	quotacheck
SRCS=	quotacheck.c fsutil.c preen.c ffs_bswap.c
MAN=	quotacheck.8
DPADD=	${LIBUTIL}
LDADD+=	-lutil
CPPFLAGS+=-I${NETBSDSRCDIR}/sbin/fsck
.PATH:	${NETBSDSRCDIR}/sbin/fsck ${NETBSDSRCDIR}/sys/ufs/ffs

.include <bsd.prog.mk>
