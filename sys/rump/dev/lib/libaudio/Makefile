#	$NetBSD: Makefile,v 1.1.4.2 2010/08/11 22:55:00 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_audio
IOCONF=	AUDIO.ioconf

SRCS=	audio.c auconv.c aurateconv.c mulaw.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
