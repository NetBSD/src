#	$NetBSD: Makefile,v 1.23 2021/09/14 20:13:03 rillig Exp $
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
CPPFLAGS+=-I${.CURDIR} -I${FSCK} -DIN_FSCK_LFS
# -DVERBOSE_BLOCKMAP 

LDADD+=-lutil
DPADD+=${LIBUTIL}

# As of 2021-09-14, lint does not recognize the types as equal, but it should.
# vnode.c(104): error: redeclaration of register_vget [27]
# vnode.h(75): previous declaration of register_vget [260]
LINT_SUPPORTED=	no

.include <bsd.prog.mk>
