
#	$NetBSD: Makefile,v 1.1.6.1 2013/01/16 05:33:57 yamt Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/dm
FILESDIR=	${TESTSDIR}

TESTS_SH=	t_dm

BINDIR=		${TESTSDIR}
MKMAN=no

PROGS=		h_dm

LDADD+= -lprop -lutil
LDADD+= -lrumpdev_disk -lrumpdev_dm
LDADD+=	-lrumpdev -lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

FILES=		t_dm_disk_1.bz2.uue t_dm_disk_2.bz2.uue t_dm_disk_3.bz2.uue

WARNS=	4
NOMAN=

.include <bsd.test.mk>
