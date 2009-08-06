#	$NetBSD: Makefile,v 1.17 2009/08/06 00:51:55 pooka Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

WARNS?=	3	# XXX: sign-compare issues

.include <bsd.own.mk>

PROG=	fsck_lfs
MAN=	fsck_lfs.8
SRCS=	bufcache.c dir.c fsutil.c inode.c lfs.c lfs_cksum.c lfs_itimes.c main.c
SRCS+=	pass0.c pass1.c pass2.c pass3.c pass4.c pass5.c pass6.c
SRCS+=	segwrite.c setup.c utilities.c vars.c vnode.c
SRCS+=	kernelops.c
FSCK=	${NETBSDSRCDIR}/sbin/fsck
.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs ${FSCK}
CPPFLAGS+=-I${.CURDIR} -I${FSCK} -DIN_FSCK_LFS "-Dmutex_enter(x)=" "-Dmutex_exit(x)="
CPPFLAGS+="-DKASSERT(x)="
# -DVERBOSE_BLOCKMAP 

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
