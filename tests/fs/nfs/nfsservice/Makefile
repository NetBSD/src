#	$NetBSD: Makefile,v 1.3 2010/07/26 19:17:37 pooka Exp $
#

NOMAN=	1
.include <bsd.own.mk>

TESTSDIR=       ${TESTSBASE}/fs/nfs/nfsservice
ATFFILE=	no

# install pre-generated /etc/exports
FILES+=			exports
FILESDIR_exports=	${TESTSDIR}

VFSTESTDIR != cd ${.CURDIR}/../../common && ${PRINTOBJDIR}
LDADD+= -L${VFSTESTDIR} -Wl,--whole-archive -lvfstest -Wl,--no-whole-archive

TESTS_C=rumpnfsd

SRCS.rumpnfsd= rumpnfsd.c nfsd.c mountd.c getmntinfo.c

LDADD+=	-lrumpfs_nfsserver -lrumpfs_nfs				# NFS support
LDADD+=	-lrumpdev_disk -lrumpdev				# disk devices
LDADD+=	-lrumpfs_ffs -lrumpvfs					# FFS
LDADD+=	-lrumpnet_netinet -lrumpnet_net -lrumpnet_local		# TCP/IP
LDADD+=	-lrumpnet_shmif						# shmif
LDADD+=	-lrumpnet -lrump -lrumpuser				# base
LDADD+=	-lpthread -lutil

CPPFLAGS+=	-DDEBUG -DMOUNT_NOMAIN -I${.CURDIR}/include -D_REENTRANT
CPPFLAGS+=	-DRUMP_SYS_NETWORKING
#CPPFLAGS+=	-DSVC_RUN_DEBUG
CPPFLAGS+=	-I${.CURDIR}/../../common/nfsrpc
DBG=-g

.include "rpcbind/Makefile.inc"
#.include "rpc/Makefile.inc"

WARNS=	2

.include <bsd.test.mk>
