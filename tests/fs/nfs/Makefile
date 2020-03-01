#	$NetBSD: Makefile,v 1.11 2020/03/01 18:08:14 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=       ${TESTSBASE}/fs/nfs

SUBDIR+= 	nfsservice

TESTS_C=	t_mountd

.for name in t_rquotad
TESTS_SH+=	${name}
TESTS_SH_SRC_${name}=	../ffs/ffs_common.sh ../ffs/quotas_common.sh ${name}.sh
.endfor

VFSTESTDIR != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
LDADD+=-L${VFSTESTDIR} -lvfstest

LDADD+=-lrumpfs_nfs                                             # NFS
LDADD+=-lrumpfs_ffs 		                                # ffs
LDADD+=-lrumpdev_disk -lrumpdev                                 # disk device
LDADD+=-lrumpnet_shmif 						# shmif
LDADD+=-lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD+=-lrumpkern_sysproxy ${LIBRUMPBASE}			# base
LDADD+=-lutil

.include <bsd.test.mk>
