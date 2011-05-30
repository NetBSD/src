#	$NetBSD: Makefile,v 1.6 2011/05/30 14:41:26 joerg Exp $
#

.include <bsd.own.mk>

TESTSDIR=       ${TESTSBASE}/fs/nfs

SUBDIR+= 	nfsservice

TESTS_C=	t_mountd

.for name in t_rquotad
TESTS_SH+=	${name}
TESTS_SH_SRC_${name}=	../ffs/quotas_common.sh ${name}.sh
.endfor

VFSTESTDIR != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
LDADD+=-L${VFSTESTDIR} -lvfstest

LDADD+=-lrumpfs_nfs                                             # NFS
LDADD+=-lrumpfs_ffs -lrumpvfs                                   # ffs
LDADD+=-lrumpdev_disk -lrumpdev                                 # disk device
LDADD+=-lrumpnet_shmif 						# shmif
LDADD+=-lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD+=-lrump -lrumpuser -lpthread                    		# base

LDADD+=-lutil

NOCLANGERROR=	# defined

.include <bsd.test.mk>
