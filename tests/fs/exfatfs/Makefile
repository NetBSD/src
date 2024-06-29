#	$NetBSD: Makefile,v 1.1.2.1 2024/06/29 19:43:26 perseant Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/exfatfs
WARNS=		4

PROGS=			h_exfatfs_server
SRCS.h_exfatfs_server=	h_exfatfs_server.c
MAN.h_exfatfs_server=	# empty
BINDIR.h_exfatfs_server=	${TESTSDIR}

TESTS_C+=	t_mount

LDADD+=-lrumpfs_exfatfs						# exfatfs
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
#VFSTESTDIR != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
#LDADD+=-L${VFSTESTDIR}
#LDADD+=-lvfstest
LDADD+=-lrumpkern_sysproxy ${LIBRUMPBASE}

.include <bsd.test.mk>
