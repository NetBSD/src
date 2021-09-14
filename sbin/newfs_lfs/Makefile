#	$NetBSD: Makefile,v 1.15 2021/09/14 20:13:03 rillig Exp $
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

# As of 2021-09-14, lint does not recognize the types as equal, but it should.
# vnode.c(104): error: redeclaration of register_vget [27]
# vnode.h(75): previous declaration of register_vget [260]
LINT_SUPPORTED=	no

.include <bsd.prog.mk>
