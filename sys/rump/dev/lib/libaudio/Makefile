#	$NetBSD: Makefile,v 1.6.10.1 2019/06/10 22:09:50 christos Exp $
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
