#	$NetBSD: Makefile,v 1.7 2007/01/09 18:15:08 pooka Exp $
#

.include <bsd.own.mk>

WARNS=		4

LIB=		puffs
SRCS=		puffs.c callcontext.c creds.c flush.c opdump.c pnode.c \
		requests.c subr.c
MAN=		puffs.3
INCS=		puffs.h puffsdump.h
INCSDIR=	/usr/include
LINTFLAGS+=-S -w
#DBG=-O0 -g

.include <bsd.lib.mk>
