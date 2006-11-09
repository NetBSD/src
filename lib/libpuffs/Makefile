#	$NetBSD: Makefile,v 1.4 2006/11/09 01:30:15 pooka Exp $
#

.include <bsd.own.mk>

WARNS=		4

LIB=		puffs
SRCS=		puffs.c subr.c creds.c opdump.c
MAN=		puffs.3
INCS=		puffs.h puffsdump.h
INCSDIR=	/usr/include
LINTFLAGS+=-S -w

.include <bsd.lib.mk>
