#	$NetBSD: Makefile,v 1.6 2019/06/20 12:14:46 isaki Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/audio
FILESDIR=	${TESTSDIR}

TESTS_SH=	t_pad

BINDIR=		${TESTSDIR}
MKMAN=no

PROGS=		h_pad

CPPFLAGS+=	-D_KERNTYPES
LDADD+=	-lrumpdev_pad -lrumpdev_audio -lrumpdev -lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser
LDADD+=	-lrump
LDADD+=	-lpthread

WARNS=	4
NOMAN=

.include <bsd.test.mk>
