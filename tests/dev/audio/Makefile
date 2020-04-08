#	$NetBSD: Makefile,v 1.5.16.1 2020/04/08 14:09:05 martin Exp $
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
LDADD+=	-lrumpdev_pad -lrumpdev_audio -lrumpdev ${LIBRUMPBASE}

FILES=		t_pad_output.bz2.uue

WARNS=	4
NOMAN=

CLEANFILES+=	t_audio.sh

t_audio.sh:	t_audio.awk audiotest.c
	${_MKTARGET_CREATE}
	${TOOL_AWK} -f ${.ALLSRC} > ${.TARGET}

.include <bsd.test.mk>
