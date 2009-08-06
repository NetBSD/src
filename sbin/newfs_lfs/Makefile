#	$NetBSD: Makefile,v 1.9 2009/08/06 00:51:55 pooka Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

WARNS?=	3	# XXX: sign-compare issues

.include <bsd.own.mk>

PROG=	newfs_lfs
SRCS=	dkcksum.c make_lfs.c lfs_cksum.c misc.c newfs.c
SRCS+=	bufcache.c vnode.c lfs.c segwrite.c lfs_itimes.c partutil.c
SRCS+=	kernelops.c
MAN=	newfs_lfs.8

DISKLABEL= ${NETBSDSRCDIR}/sbin/disklabel
FSCK_LFS= ${NETBSDSRCDIR}/sbin/fsck_lfs
FSCK= ${NETBSDSRCDIR}/sbin/fsck

.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs ${DISKLABEL} ${FSCK_LFS} ${FSCK}

DPADD=  ${LIBUTIL}
LDADD=  -lutil

LDADD+=-lprop
DPADD+=${LIBPROP}

CPPFLAGS+=-I${FSCK_LFS} -I${FSCK} "-Dmutex_enter(x)=" "-Dmutex_exit(x)=" # -DNDEBUG # -DVERBOSE_BLOCKMAP
CPPFLAGS+="-DKASSERT(x)="

.include <bsd.prog.mk>
