#	$NetBSD: Makefile,v 1.6 2017/06/01 09:44:30 pgoyette Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_audio
COMMENT=Audio support (incl. /dev/audio and /dev/mixer)

IOCONF=	AUDIO.ioconf

SRCS=	audio.c auconv.c aurateconv.c auvolconv.c mulaw.c
SRCS+=	audio_component.c

CPPFLAGS+=	-DNAUDIO=1 -DNAURATECONV=1 -DNMULAW=1

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
