#	$NetBSD: Makefile,v 1.21 2019/09/29 23:45:01 mrg Exp $
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
.if defined(HAVE_GCC) && ${HAVE_GCC} >= 7 && ${ACTIVE_CC} == "gcc"
COPTS.quotacheck.c+=	-Wno-error=implicit-fallthrough
.endif

.include <bsd.prog.mk>
