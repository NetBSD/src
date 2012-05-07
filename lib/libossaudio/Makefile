#	$NetBSD: Makefile,v 1.8.8.1 2012/05/07 16:55:33 riz Exp $

WARNS=	5

LIB=	ossaudio
MAN=	ossaudio.3

SRCS=	ossaudio.c

CPPFLAGS+= -I${.CURDIR}

INCS=	soundcard.h
INCSDIR=/usr/include

.include <bsd.lib.mk>
