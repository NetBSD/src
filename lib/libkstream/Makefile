#	$NetBSD: Makefile,v 1.2.4.2 2000/06/23 16:16:45 minoura Exp $

LIB=	kstream

SRCS=	kstream.c kstream-des.c

CPPFLAGS+=-I${DESTDIR}/usr/include/kerberosIV

INCS=	kstream.h
INCSDIR=/usr/include/kerberosIV

SHLIB_MAJOR?= 1
SHLIB_MINOR?= 0

.include <bsd.lib.mk>
