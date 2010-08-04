#	$NetBSD: Makefile,v 1.1 2010/08/04 13:15:15 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/audio
FILESDIR=	${TESTSDIR}

TESTS_SH=	t_pad
TESTS_C=	h_pad

ATFFILE=	yes	# for h_pad ... or rather not for it

LDADD+=	-lrumpdev_pad -lrumpdev_audio -lrumpdev -lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

FILES=		t_pad_output.bz2

WARNS=	4
NOMAN=

.include <bsd.test.mk>
