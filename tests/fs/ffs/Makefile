#	$NetBSD: Makefile,v 1.15 2011/03/06 17:08:39 bouyer Exp $
#

.include <bsd.own.mk>

SUBDIR=		clients

TESTSDIR=	${TESTSBASE}/fs/ffs
WARNS=		4

PROGS=			h_quota2_server h_quota2_tests
SRCS.h_quota2_server=	h_quota2_server.c
MAN.h_quota2_server=	# empty
BINDIR.h_quota2_server=	${TESTSDIR}

SRCS.h_quota2_tests=	h_quota2_tests.c
MAN.h_quota2_tests=	# empty
BINDIR.h_quota2_tests=	${TESTSDIR}

.for name in t_getquota t_setquota t_quotalimit t_clearquota t_miscquota
TESTS_SH+=	${name}
TESTS_SH_SRC_${name}=	quotas_common.sh ${name}.sh
.endfor

TESTS_C+=	t_fifos
TESTS_C+=	t_snapshot
TESTS_C+=	t_snapshot_log
TESTS_C+=	t_snapshot_v2
TESTS_C+=	t_mount
TESTS_C+=	t_quota2_1
TESTS_C+=	t_quota2_remount

LDADD+=-lrumpfs_ffs						# ffs
LDADD+=-lrumpdev_fss						# snapshot dev
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
LDADD+=-lrumpvfs_fifofs -lrumpnet_local -lrumpnet_net -lrumpnet	# fifos
VFSTESTDIR != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
LDADD+=-L${VFSTESTDIR} -lvfstest
LDADD+=-lrumpvfs -lrump -lrumpuser -lpthread			# base

.include <bsd.test.mk>
