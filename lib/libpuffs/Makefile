#	$NetBSD: Makefile,v 1.5 2006/12/07 23:15:20 pooka Exp $
#

.include <bsd.own.mk>

WARNS=		4

LIB=		puffs
SRCS=		puffs.c subr.c creds.c opdump.c requests.c
MAN=		puffs.3
INCS=		puffs.h puffsdump.h
INCSDIR=	/usr/include
LINTFLAGS+=-S -w

.include <bsd.lib.mk>
