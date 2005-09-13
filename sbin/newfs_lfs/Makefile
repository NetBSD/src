#	$NetBSD: Makefile,v 1.4 2005/09/13 04:40:25 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/18/93

.include <bsd.own.mk>

PROG=	newfs_lfs
SRCS=	dkcksum.c make_lfs.c lfs_cksum.c misc.c newfs.c
SRCS+=	bufcache.c vnode.c lfs.c segwrite.c lfs_itimes.c
MAN=	newfs_lfs.8
.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs ${NETBSDSRCDIR}/sbin/disklabel ${NETBSDSRCDIR}/sbin/fsck_lfs

FSCK_LFS= ${NETBSDSRCDIR}/sbin/fsck_lfs
DPADD=  ${LIBUTIL}
LDADD=  -lutil
CPPFLAGS+=-I${FSCK_LFS} # -DNDEBUG # -DVERBOSE_BLOCKMAP

.include <bsd.prog.mk>
