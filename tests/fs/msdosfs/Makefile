#	$NetBSD: Makefile,v 1.3 2020/03/01 18:08:13 christos Exp $
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
