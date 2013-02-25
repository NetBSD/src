#	$NetBSD: Makefile,v 1.3.12.1 2013/02/25 00:30:20 tls Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/audio
FILESDIR=	${TESTSDIR}

TESTS_SH=	t_pad

BINDIR=		${TESTSDIR}
MKMAN=no

PROGS=		h_pad

LDADD+=	-lrumpdev_pad -lrumpdev_audio -lrumpdev -lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser
LDADD+=	-lrump
LDADD+=	-lpthread

FILES=		t_pad_output.bz2.uue

WARNS=	4
NOMAN=

.include <bsd.test.mk>
