#	$NetBSD: Makefile,v 1.2.6.1 2013/01/16 05:33:57 yamt Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/md
FILESDIR=	${TESTSDIR}

TESTS_SH=	t_md

BINDIR=		${TESTSDIR}
MKMAN=no

PROGS=		h_mdserv

LDADD+=	-lrumpdev_md -lrumpdev_disk -lrumpdev -lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser
LDADD+=	-lpthread

WARNS=	4
NOMAN=

.include <bsd.test.mk>
