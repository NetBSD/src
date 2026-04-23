#	$NetBSD: Makefile.inc,v 1.9 2026/04/23 16:26:05 perseant Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

WARNS?=3		# XXX: too many sign-compare issues

.include <bsd.own.mk>

SRCS+=	lfs_cleanerd.c coalesce.c cleansrv.c
SRCS+=	lfs_cksum.c
SRCS+=	bufcache.c vnode.c lfs.c kernelops.c # segwrite.c
.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs ${NETBSDSRCDIR}/sbin/fsck_lfs	\
	${NETBSDSRCDIR}/libexec/lfs_cleanerd

COPTS.lfs_cleanerd.c+=	${CC_WNO_STRINGOP_TRUNCATION}

FSCK_LFS= ${NETBSDSRCDIR}/sbin/fsck_lfs
DPADD+=  ${LIBUTIL}
LDADD+=  -lutil
CPPFLAGS+=-I${FSCK_LFS} -I${NETBSDSRCDIR}/sbin/mount_lfs # -DUSE_CLIENT_SERVER 
#CPPFLAGS+=-DREPAIR_ZERO_FINFO
#CPPFLAGS+=-DTEST_PATTERN
