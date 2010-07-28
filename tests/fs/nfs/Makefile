#	$NetBSD: Makefile,v 1.3 2010/07/28 19:23:01 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=       ${TESTSBASE}/fs/nfs

SUBDIR+= 	nfsservice

TESTS_C=	t_mountd

LDADD+=-lrumpfs_ffs                                             # ffs
LDADD+=-lrumpfs_nfs                                             # NFS
LDADD+=-lrumpdev_disk -lrumpdev                                 # disk device
LDADD+=-lrumpnet_shmif -lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD+=-lrumpvfs -lrump -lrumpuser -lpthread                    # base

LDADD+=-lutil

VFSTESTDIR != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
LDADD+=-L${VFSTESTDIR} -lvfstest

.include <bsd.test.mk>
