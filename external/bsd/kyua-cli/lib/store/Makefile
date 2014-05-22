# $NetBSD: Makefile,v 1.1.12.2 2014/05/22 15:45:31 yamt Exp $

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
