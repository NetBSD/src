#	$NetBSD: Makefile,v 1.9.2.2 2007/08/15 13:50:35 skrll Exp $
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
SRCS+=	clock_subr.c param.c subr_hash.c subr_prf_bitmask.c		\
	subr_specificdata.c subr_time.c subr_xxx.c sync_subr.c		\
	vfs_bio.c vfs_cache.c vfs_vnops.c vfs_init.c vfs_subr2.c	\
	vnode_if.c param.c

# src/lib/libkern
SRCS+=	__assert.c scanc.c skpc.c

RUMUSEROBJDIR != cd ${.CURDIR}/../rumpuser && ${PRINTOBJDIR}
OBJS+=	${RUMUSEROBJDIR}/rumpuser.o

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/common/include	\
		-I${NETBSDSRCDIR}/sys/rump/librump/rumpuser		\
		-I${.CURDIR}/opt -DMAXUSERS=32

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
