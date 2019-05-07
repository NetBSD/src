#	$NetBSD: Makefile,v 1.3.2.1 2019/05/07 15:01:50 isaki Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/audio

KMOD=	audio
IOCONF=	audio.ioconf
SRCS=	audio.c \
	alaw.c \
	linear.c \
	mulaw.c

CPPFLAGS+=	-DNAUDIO=1

WARNS=	3

.include <bsd.kmodule.mk>
