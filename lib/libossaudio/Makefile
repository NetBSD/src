#	$NetBSD: Makefile,v 1.11 2021/06/08 18:43:54 nia Exp $

WARNS=	5

LIB=	ossaudio
MAN=	ossaudio.3

SRCS=	oss_caps.c oss_dsp.c oss_ioctl.c
SRCS+=	oss3_mixer.c oss4_mixer.c
SRCS+=	oss4_global.c

CPPFLAGS+= -I${.CURDIR}

INCS=	soundcard.h
INCSDIR=/usr/include

.include <bsd.lib.mk>
