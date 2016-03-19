#	$NetBSD: Makefile,v 1.3.2.2 2016/03/19 11:30:34 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_audio
COMMENT=Audio support (incl. /dev/audio and /dev/mixer)

IOCONF=	AUDIO.ioconf

SRCS=	audio.c auconv.c aurateconv.c auvolconv.c mulaw.c
SRCS+=	audio_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
