#	$NetBSD: Makefile,v 1.27 2008/01/25 06:42:29 pooka Exp $
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"

LIB=	rump

.PATH:	${NETBSDSRCDIR}/sys/kern ${NETBSDSRCDIR}/sys/lib/libkern	\
	${NETBSDSRCDIR}/sys/conf ${NETBSDSRCDIR}/sys/dev		\
	${NETBSDSRCDIR}/sys/miscfs/genfs				\
	${NETBSDSRCDIR}/sys/miscfs/syncfs

# implement something
SRCS=	rump.c atomic.c auth.c emul.c intr.c genfs_io.c locks.c		\
	ltsleep.c pool.c specfs.c vfs.c vm.c

# just stubs
SRCS+=	fstrans_stub.c misc_stub.c pmap_stub.c vfsops_stub.c

# sys/kern
SRCS+=	clock_subr.c kern_descrip.c kern_lock.c kern_stub.c param.c	\
	subr_bufq.c subr_hash.c subr_prf2.c subr_specificdata.c		\
	subr_time.c subr_workqueue.c sys_generic.c vfs_bio.c		\
	vfs_cache.c vfs_getcwd.c vfs_vnops.c vfs_init.c vfs_lookup.c	\
	vfs_subr2.c vfs_getcwd.c vnode_if.c

# sys/miscfs
SRCS+=	genfs_vnops.c sync_subr.c

# src/kern bufq
SRCS+=	subr_bufq.c bufq_disksort.c bufq_fcfs.c bufq_priocscan.c	\
	bufq_readprio.c

# src/lib/libkern
SRCS+=	__assert.c scanc.c skpc.c

SRCS+=	rumpvnode_if.c

RUMPUSEROBJDIR != cd ${.CURDIR}/../rumpuser && ${PRINTOBJDIR}
OBJS+=	${RUMPUSEROBJDIR}/rumpuser.o ${RUMPUSEROBJDIR}/rumpuser_pth.o

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/common/include	\
		-I${NETBSDSRCDIR}/sys/rump/librump/rumpuser		\
		-I${.CURDIR}/opt -DMAXUSERS=32
#CPPFLAGS+=	-DRUMP_WITHOUT_THREADS

CFLAGS+=	-Wno-pointer-sign

# Create a few files.  We can't include them directly, because that
# would create hideous namespace lossage.  So just do some clever
# (or less clever) renaming.
#
# Yes, this is borderline disgusting not to mention seriously
# labile, but what are you gonna do for a quick hack?
#
VOPTORUMPVOP="-e s/vop/rump_vop/g;/VOPARG/!s/VOP/RUMP_VOP/g;/vfs_op_desc/,\$$d"
VOPTORUMPVOP+="-e s/vm_prot_t/int/g"
VOPTORUMPVOP+="-e s/voff_t/off_t/g;/sys\/buf\.h/s/.*/struct buf;/"
RVNH=\#include \"rump.h\"
rumpvnode_if.c: vnode_if.c
	sed ${VOPTORUMPVOP}";s/SYS_VNODE_IF_H/SYS_RUMPVNODE_IF_H/g"	\
	    < ${NETBSDSRCDIR}/sys/sys/vnode_if.h > rumpvnode_if.h
	# XXX: some creative kludging to simulate 'a\' (I have no
	# clue how to make it work in a Makefile
	sed ${VOPTORUMPVOP}";/sys\/vnode.h/{x;s/.*/${RVNH}/;x;G;n;}"	\
	    < ${NETBSDSRCDIR}/sys/kern/vnode_if.c > rumpvnode_if.c
	sed -n '/define/s/NAMEI_/RUMP_NAMEI_/p'				\
	    < ${NETBSDSRCDIR}/sys/sys/namei.h > rumpdefs.h
	printf "#ifndef __VTYPE_DEFINED\n#define __VTYPE_DEFINED\n"	\
	    >> rumpdefs.h
	sed -n '/enum vtype.*{/p'					\
	    < ${NETBSDSRCDIR}/sys/sys/vnode.h >> rumpdefs.h
	printf "#endif /* __VTYPE_DEFINED */\n" >> rumpdefs.h
	printf "#ifndef EJUSTRETURN\n" >> rumpdefs.h
	sed -n '/EJUSTRETURN/p'						\
	    < ${NETBSDSRCDIR}/sys/sys/errno.h >> rumpdefs.h
	printf "#endif /* EJUSTRETURN */\n" >> rumpdefs.h

CLEANFILES+=	rumpvnode_if.c rumpvnode_if.h rumpdefs.h

beforedepend: rumpvnode_if.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
