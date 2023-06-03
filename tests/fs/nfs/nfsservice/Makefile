#	$NetBSD: Makefile,v 1.17 2023/06/03 09:09:15 lukem Exp $
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

SRCS.rumpnfsd= rumpnfsd.c nfsd.c mountd.c getmntinfo.c get_net.c

LDADD+=	-lrumpnet -lrumpnet_net -lrumpnet_local
LDADD+= -lrumpnet_netinet6 -lrumpnet_netinet			# TCP/IP
LDADD+=	-lrumpfs_nfsserver -lrumpfs_nfs				# NFS support
LDADD+=	-lrumpdev_disk -lrumpdev				# disk devices
LDADD+=	-lrumpfs_ffs 						# FFS
LDADD+=	-lrumpnet_shmif						# shmif
LDADD+=	-lrumpnet -lrumpkern_sysproxy ${LIBRUMPBASE}		# base
LDADD+=	-lutil

CPPFLAGS+=	-DDEBUG -DMOUNT_NOMAIN -D_REENTRANT
CPPFLAGS+=	-DRUMP_SYS_NETWORKING -DMOUNTD_RUMP -DNFSD_RUMP

.PATH.c: ${NETBSDSRCDIR}/usr.sbin/mountd ${NETBSDSRCDIR}/usr.sbin/nfsd

# RPCBIND
.PATH.c: ${NETBSDSRCDIR}/usr.sbin/rpcbind
SRCS.rumpnfsd+=	check_bound.c rpcb_stat.c rpcb_svc_4.c rpcbind.c pmap_svc.c \
		rpcb_svc.c rpcb_svc_com.c security.c util.c 

LIBRPCDIR=        ${NETBSDSRCDIR}/lib/libc/rpc

CPPFLAGS+=	-I${LIBRPCDIR} -DPORTMAP -DLIBWRAP -DRPCBIND_RUMP

# Uncomment these to get any useful output from 'rpcbind -d'
# CPPFLAGS+=	-DRPCBIND_DEBUG
# CPPFLAGS+=	-DSVC_RUN_DEBUG

LDADD+= -lwrap -lblocklist -lutil
DPADD+= ${LIBWRAP} ${LIBBLOCKLIST} ${LIBUTIL}

SANITIZER_RENAME_SYMBOL+=	__getmntinfo13

COPTS.pmap_svc.c+=	${CC_WNO_CAST_FUNCTION_TYPE}
COPTS.rpcb_svc.c+=	${CC_WNO_CAST_FUNCTION_TYPE}
COPTS.rpcb_svc_4.c+=	${CC_WNO_CAST_FUNCTION_TYPE}
COPTS.rpcb_svc_com.c+=	${CC_WNO_CAST_FUNCTION_TYPE}
COPTS.mountd.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.test.mk>
