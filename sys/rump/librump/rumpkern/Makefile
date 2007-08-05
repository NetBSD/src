#	$NetBSD: Makefile,v 1.1.2.2 2007/08/05 22:28:08 pooka Exp $
#

.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"
.include <bsd.own.mk>

LIB=	rump

.PATH:  ${NETBSDSRCDIR}/sys/kern ${NETBSDSRCDIR}/sys/lib/libkern

# implements something
SRCS=	rump.c buffercache.c emul.c vfs.c genfs.c vm.c pool.c specfs.c

# just stubs
SRCS+=	fstrans_stub.c kauth_stub.c lock_stub.c misc_stub.c vfsops_stub.c

# sys/kern
SRCS+=	subr_hash.c subr_prf_bitmask.c subr_xxx.c vfs_init.c		\
	vfs_cache.c vfs_subr2.c vnode_if.c

# src/lib/libkern
SRCS+=	scanc.c skpc.c

RUMUSEROBJDIR != cd ${.CURDIR}/../rumpuser && ${PRINTOBJDIR}
OBJS+=	${RUMUSEROBJDIR}/rumpuser.o

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/common/include	\
		-I${NETBSDSRCDIR}/sys/rump/librump/rumpuser

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
