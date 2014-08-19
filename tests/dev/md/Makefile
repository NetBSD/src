#	$NetBSD: Makefile,v 1.2.12.2 2014/08/20 00:04:46 tls Exp $
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
LDADD+=	-lrump
LDADD+=	-lpthread

WARNS=	4
NOMAN=

.include <bsd.test.mk>
