#	$NetBSD: Makefile,v 1.9.12.1 2013/06/23 06:28:52 tls Exp $
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

CPPFLAGS+=-I${FSCK_LFS} -I${FSCK} # -DNDEBUG # -DVERBOSE_BLOCKMAP

.include <bsd.prog.mk>
