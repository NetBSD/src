# $NetBSD: Makefile,v 1.1 2015/09/24 14:15:59 christos Exp $

.include <bsd.own.mk>

LIB=		rtld_db
SHLIB_MAJOR= 	0
SHLIB_MINOR=	0
MAN=		librtld_db.3

SRCS=		rtld_db.c
INCS=		rtld_db.h
INCSDIR=	/usr/include

.PATH:	${.CURDIR}/../dist

CPPFLAGS+= -I${.CURDIR} -I${.CURDIR}/../libproc

LIBDPLIBS+=	util	${NETBSDSRCDIR}/lib/libutil

.include <bsd.lib.mk>
