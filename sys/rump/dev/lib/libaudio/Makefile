#	$NetBSD: Makefile,v 1.5 2016/01/26 23:12:14 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_audio
COMMENT=Audio support (incl. /dev/audio and /dev/mixer)

IOCONF=	AUDIO.ioconf

SRCS=	audio.c auconv.c aurateconv.c auvolconv.c mulaw.c
SRCS+=	audio_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
