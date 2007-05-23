#	$NetBSD: Makefile,v 1.2 2007/05/23 14:41:02 christos Exp $

USE_SHLIBDIR=	yes
WARNS=4

CPPFLAGS+=-I${.CURDIR}/ssp
LIB=	ssp
SRCS=	gets_chk.c fgets_chk.c memcpy_chk.c memmove_chk.c memset_chk.c \
	snprintf_chk.c sprintf_chk.c strcat_chk.c strcpy_chk.c \
	strncat_chk.c strncpy_chk.c vsnprintf_chk.c vsprintf_chk.c

INCS=	ssp/ssp.h ssp/stdio.h ssp/string.h ssp/unistd.h
INCSDIR=/usr/include
MAN=	ssp.3 __builtin_object_size.3

.include <bsd.own.mk>
.include <bsd.shlib.mk>
.include <bsd.lib.mk>
