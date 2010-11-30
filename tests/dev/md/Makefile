#	$NetBSD: Makefile,v 1.2 2010/11/30 19:03:44 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/md
FILESDIR=	${TESTSDIR}

TESTS_SH=	t_md
TESTS_C=	h_mdserv

ATFFILE=	yes

LDADD+=	-lrumpdev_md -lrumpdev_disk -lrumpdev -lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser
LDADD+=	-lpthread

WARNS=	4
NOMAN=

.include <bsd.test.mk>
