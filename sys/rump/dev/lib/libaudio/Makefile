#	$NetBSD: Makefile,v 1.2 2014/03/13 01:57:52 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_audio
IOCONF=	AUDIO.ioconf

SRCS=	audio.c auconv.c aurateconv.c mulaw.c
SRCS+=	audio_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
