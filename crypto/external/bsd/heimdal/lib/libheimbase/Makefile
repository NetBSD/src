# $NetBSD: Makefile,v 1.1.20.1 2017/08/30 06:57:45 snj Exp $

USE_FORT?= yes	# network protocol library

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/lib/base

LIB=		heimbase

SRCS=			\
	array.c		\
	bsearch.c	\
	db.c		\
	data.c		\
	dict.c		\
	error.c		\
	json.c		\
	number.c	\
	bool.c		\
	heimbase.c	\
	null.c		\
	string.c

INCSDIR=	/usr/include/krb5
INCS=		heimbase.h

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.lib.mk>
