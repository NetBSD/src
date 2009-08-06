#	$NetBSD: Makefile.inc,v 1.1 2009/08/06 00:51:55 pooka Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

WARNS?=3		# XXX: too many sign-compare issues

.include <bsd.own.mk>

SRCS=	lfs_cleanerd.c fdfs.c coalesce.c cleansrv.c
SRCS+=  lfs_cksum.c
SRCS+=	bufcache.c vnode.c lfs.c kernelops.c # segwrite.c
.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs ${NETBSDSRCDIR}/sbin/fsck_lfs	\
	${NETBSDSRCDIR}/libexec/lfs_cleanerd

FSCK_LFS= ${NETBSDSRCDIR}/sbin/fsck_lfs
DPADD=  ${LIBUTIL}
LDADD=  -lutil
CPPFLAGS+=-I${FSCK_LFS} -I${NETBSDSRCDIR}/sbin/mount_lfs # -DUSE_CLIENT_SERVER 
CPPFLAGS+="-Dmutex_enter(x)=" "-Dmutex_exit(x)="
CPPFLAGS+="-DKASSERT(x)="
#CPPFLAGS+=-DREPAIR_ZERO_FINFO
#CPPFLAGS+=-DTEST_PATTERN
