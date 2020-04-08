#	$NetBSD: Makefile,v 1.3.16.1 2020/04/08 14:09:05 martin Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/dm
FILESDIR=	${TESTSDIR}

TESTS_SH=	t_dm

BINDIR=		${TESTSDIR}
MKMAN=no

PROGS=		h_dm

CPPFLAGS+=-D_KERNTYPES
LDADD+= -lprop -lutil
LDADD+= -lrumpdev_disk -lrumpdev_dm ${LIBRUMPBASE}

FILES=		t_dm_disk_1.bz2.uue t_dm_disk_2.bz2.uue t_dm_disk_3.bz2.uue

WARNS=	4
NOMAN=

.include <bsd.test.mk>
