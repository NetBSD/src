#	$NetBSD: Makefile,v 1.2.16.1 2020/04/08 14:09:07 martin Exp $
#

TESTSDIR=	${TESTSBASE}/fs/msdosfs
WARNS=		4

TESTS_C=	t_snapshot

LDADD+=-lrumpfs_msdos -lrumpfs_tmpfs				# fs drivers
LDADD+=-lrumpdev_fss						# snapshot dev
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
LDADD+=-lrumpkern_sysproxy ${LIBRUMPBASE}
LDADD+=-lpthread

.include <bsd.test.mk>
