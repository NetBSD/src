#	$NetBSD: Makefile,v 1.7 2019/05/08 13:40:19 isaki Exp $
#

.PATH:	${.CURDIR}/../../../../dev/audio

LIB=	rumpdev_audio
COMMENT=Audio support (incl. /dev/audio and /dev/mixer)

IOCONF=	AUDIO.ioconf

SRCS=	alaw.c audio.c linear.c mulaw.c
SRCS+=	audio_component.c

CPPFLAGS+=	-DNAUDIO=1

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
