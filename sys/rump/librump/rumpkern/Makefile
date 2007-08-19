#	$NetBSD: Makefile,v 1.11 2007/08/19 21:24:22 pooka Exp $
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"

LIB=	rump

.PATH:	${NETBSDSRCDIR}/sys/kern ${NETBSDSRCDIR}/sys/lib/libkern	\
	${NETBSDSRCDIR}/sys/conf ${NETBSDSRCDIR}/sys/dev		\
	${NETBSDSRCDIR}/sys/miscfs/syncfs

# implements something
SRCS=	rump.c emul.c vfs.c genfs.c vm.c pool.c specfs.c

# just stubs
SRCS+=	fstrans_stub.c kauth_stub.c lock_stub.c misc_stub.c		\
	pmap_stub.c vfsops_stub.c

# sys/kern
SRCS+=	clock_subr.c kern_stub.c param.c subr_hash.c subr_prf2.c	\
	subr_specificdata.c subr_time.c sync_subr.c			\
	vfs_bio.c vfs_cache.c vfs_vnops.c vfs_init.c vfs_subr2.c	\
	vnode_if.c param.c

# src/lib/libkern
SRCS+=	__assert.c scanc.c skpc.c

SRCS+=	rumpvnode_if.c

RUMUSEROBJDIR != cd ${.CURDIR}/../rumpuser && ${PRINTOBJDIR}
OBJS+=	${RUMUSEROBJDIR}/rumpuser.o

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/common/include	\
		-I${NETBSDSRCDIR}/sys/rump/librump/rumpuser		\
		-I${.CURDIR}/opt -DMAXUSERS=32

# Create a few files.  We can't include them directly, because that
# would create hideous namespace lossage.  So just do some clever
# (or less clever) renaming.
#
VOPTORUMPVOP=s/vop/rump_vop/g;/VOPARG/!s/VOP/RUMP_VOP/g;/vfs_op_desc/,\$$d
RVNH=\#include \"rumpvnode_if.h\"
rumpvnode_if.c: vnode_if.c
	sed "${VOPTORUMPVOP};s/SYS_VNODE_IF_H/SYS_RUMPVNODE_IF_H/g"	\
	    < ${NETBSDSRCDIR}/sys/sys/vnode_if.h > rumpvnode_if.h
	# XXX: some creative kludging to simulate 'a\' (I have no
	# clue how to make it work in a Makefile
	sed "${VOPTORUMPVOP};/sys\/vnode.h/{x;s/.*/${RVNH}/;x;G;n;}"	\
	    < ${NETBSDSRCDIR}/sys/kern/vnode_if.c > rumpvnode_if.c
	sed -n '/define/s/NAMEI_/RUMP_NAMEI_/p'				\
	    < ${NETBSDSRCDIR}/sys/sys/namei.h > rumpdefs.h

CLEANFILES+=	rumpvnode_if.c rumpvnode_if.h rumpdefs.h

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
