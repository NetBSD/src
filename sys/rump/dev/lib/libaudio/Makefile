#	$NetBSD: Makefile,v 1.1.14.1 2014/05/22 11:41:11 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_audio
IOCONF=	AUDIO.ioconf

SRCS=	audio.c auconv.c aurateconv.c mulaw.c
SRCS+=	audio_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
