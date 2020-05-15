#	$NetBSD: Makefile,v 1.22 2020/05/15 23:32:28 christos Exp $
#

RUMPFIFO=yes	# use the real rump fifofs
.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/ffs
WARNS=		4

PROGS=			h_ffs_server h_quota2_tests
SRCS.h_ffs_server=	h_ffs_server.c
MAN.h_ffs_server=	# empty
BINDIR.h_ffs_server=	${TESTSDIR}

SRCS.h_quota2_tests=	h_quota2_tests.c
MAN.h_quota2_tests=	# empty
BINDIR.h_quota2_tests=	${TESTSDIR}

.for name in t_getquota t_setquota t_quotalimit t_clearquota t_miscquota
TESTS_SH+=	${name}
TESTS_SH_SRC_${name}=	ffs_common.sh quotas_common.sh ${name}.sh
.endfor

TESTS_C+=	t_extattr
TESTS_C+=	t_fifos
TESTS_C+=	t_snapshot
TESTS_C+=	t_snapshot_log
TESTS_C+=	t_snapshot_v2
TESTS_C+=	t_mount
TESTS_C+=	t_quota2_1
TESTS_C+=	t_quota2_remount
TESTS_C+=	t_update_log

LDADD+=-lrumpfs_ffs						# ffs
LDADD+=-lrumpdev_fss						# snapshot dev
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
VFSTESTDIR != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
LDADD+=-L${VFSTESTDIR} -lvfstest
LDADD+=-lrumpkern_sysproxy ${LIBRUMPBASE}

.include <bsd.test.mk>
