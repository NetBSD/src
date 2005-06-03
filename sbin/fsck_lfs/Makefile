#	$NetBSD: Makefile,v 1.9 2005/06/03 18:02:03 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	fsck_lfs
MAN=	fsck_lfs.8
SRCS=	bufcache.c dir.c fsutil.c inode.c lfs.c lfs_cksum.c main.c
SRCS+=	pass0.c pass1.c pass2.c pass3.c pass4.c pass5.c pass6.c
SRCS+=	segwrite.c setup.c utilities.c vars.c vnode.c
FSCK=	${NETBSDSRCDIR}/sbin/fsck
.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs ${FSCK}
CPPFLAGS+=-I${FSCK} # -DNDEBUG # -DVERBOSE_BLOCKMAP

COPTS.dir.c+=	-Wno-uninitialized	# XXX: PR 30394
COPTS.pass6.c+=	-Wno-uninitialized	# XXX: PR 30411

.include <bsd.prog.mk>
