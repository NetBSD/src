#	$NetBSD: Makefile,v 1.10 2012/05/05 15:57:45 christos Exp $

WARNS=	5

LIB=	ossaudio
MAN=	ossaudio.3

SRCS=	ossaudio.c

CPPFLAGS+= -I${.CURDIR}

INCS=	soundcard.h
INCSDIR=/usr/include

.include <bsd.lib.mk>
