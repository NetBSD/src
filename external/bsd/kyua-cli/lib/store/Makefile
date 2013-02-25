# $NetBSD: Makefile,v 1.1.4.2 2013/02/25 00:26:20 tls Exp $

KYUA_LIBS=	utils

.include <bsd.init.mk>

.PATH:		${SRCDIR}/store

LIB=		store

WARNS?=		3

SRCS=		backend.cpp \
		dbtypes.cpp \
		exceptions.cpp \
		metadata.cpp \
		transaction.cpp

.include <bsd.lib.mk>
