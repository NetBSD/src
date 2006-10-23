#	$NetBSD: Makefile,v 1.3 2006/10/23 00:22:24 christos Exp $
#

.include <bsd.own.mk>

WARNS=		4

LIB=		puffs
SRCS=		puffs.c subr.c creds.c opdump.c
INCS=		puffs.h puffsdump.h
INCSDIR=	/usr/include
LINTFLAGS+=-S -w

.include <bsd.lib.mk>
