#	$NetBSD: Makefile,v 1.10 2007/01/19 21:10:55 pooka Exp $
#

.include <bsd.own.mk>

WARNS=		4

LIB=		puffs
SRCS=		puffs.c callcontext.c creds.c flush.c null.c opdump.c	\
		paths.c pnode.c requests.c subr.c
MAN=		puffs.3 puffs_cc.3 puffs_flush.3 puffs_node.3 puffs_path.3
INCS=		puffs.h puffsdump.h
INCSDIR=	/usr/include
LINTFLAGS+=-S -w
#DBG=-O0 -g

.include <bsd.lib.mk>
