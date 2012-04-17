#	$NetBSD: Makefile,v 1.8.6.1 2012/04/17 00:05:29 yamt Exp $

WARNS=	2

LIB=	ossaudio
MAN=	ossaudio.3

SRCS=	ossaudio.c

CPPFLAGS+= -I${.CURDIR}

INCS=	soundcard.h
INCSDIR=/usr/include

.include <bsd.lib.mk>
