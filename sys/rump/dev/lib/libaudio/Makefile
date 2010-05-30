#	$NetBSD: Makefile,v 1.1.2.2 2010/05/30 05:18:04 rmind Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_audio
IOCONF=	AUDIO.ioconf

SRCS=	audio.c auconv.c aurateconv.c mulaw.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
