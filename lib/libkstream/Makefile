#	$NetBSD: Makefile,v 1.2 2000/06/17 06:39:32 thorpej Exp $

LIB=	kstream

SRCS=	kstream.c kstream-des.c

CPPFLAGS+=-I${DESTDIR}/usr/include/kerberosIV

INCS=	kstream.h
INCSDIR=/usr/include/kerberosIV

SHLIB_MAJOR?= 1
SHLIB_MINOR?= 0

.include <bsd.lib.mk>
