#	$NetBSD: Makefile,v 1.22 2023/06/03 21:30:24 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	quotacheck
SRCS=	quotacheck.c fsutil.c preen.c ffs_bswap.c
MAN=	quotacheck.8
DPADD=	${LIBUTIL}
LDADD+=	-lutil
CPPFLAGS+=-I${NETBSDSRCDIR}/sbin/fsck -I${NETBSDSRCDIR}/usr.bin/quota
.PATH:	${NETBSDSRCDIR}/sbin/fsck ${NETBSDSRCDIR}/sys/ufs/ffs ${NETBSDSRCDIR}/usr.bin/quota
SRCS+=  quotautil.c

.include <bsd.own.mk>

# Has fallthru comment that is missed
COPTS.quotacheck.c+=	${CC_WNO_IMPLICIT_FALLTHROUGH}

.include <bsd.prog.mk>
