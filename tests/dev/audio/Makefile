#	$NetBSD: Makefile,v 1.7 2020/02/11 07:03:16 isaki Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/audio
FILESDIR=	${TESTSDIR}

PROGS+=		audiotest
PROGS+=		h_pad

SRCS.audiotest=	audiotest.c
LDADD.audiotest+=	-lutil

SRCS.h_pad=	h_pad.c

TESTS_SH+=	t_audio
TESTS_SH+=	t_pad

BINDIR=		${TESTSDIR}
MKMAN=no

CPPFLAGS+=	-D_KERNTYPES
LDADD+=	-lrumpdev_pad -lrumpdev_audio -lrumpdev -lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser
LDADD+=	-lrump
LDADD+=	-lpthread

WARNS=	4
NOMAN=

CLEANFILES+=	t_audio.sh

t_audio.sh:	t_audio.awk audiotest.c
	${_MKTARGET_CREATE}
	${TOOL_AWK} -f ${.ALLSRC} > ${.TARGET}

.include <bsd.test.mk>
