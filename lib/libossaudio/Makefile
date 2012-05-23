#	$NetBSD: Makefile,v 1.8.6.2 2012/05/23 10:07:32 yamt Exp $

WARNS=	5

LIB=	ossaudio
MAN=	ossaudio.3

SRCS=	ossaudio.c

CPPFLAGS+= -I${.CURDIR}

INCS=	soundcard.h
INCSDIR=/usr/include

.include <bsd.lib.mk>
