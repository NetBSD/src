#	$NetBSD: Makefile,v 1.9 2012/03/21 10:08:30 matt Exp $

WARNS=	2

LIB=	ossaudio
MAN=	ossaudio.3

SRCS=	ossaudio.c

CPPFLAGS+= -I${.CURDIR}

INCS=	soundcard.h
INCSDIR=/usr/include

.include <bsd.lib.mk>
