#	$NetBSD: Makefile,v 1.2.8.1 2019/06/10 22:09:11 christos Exp $

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
