#	$NetBSD: Makefile,v 1.11 2007/01/20 15:09:34 pooka Exp $
#

.include <bsd.own.mk>

WARNS=		4

LIB=		puffs
SRCS=		puffs.c callcontext.c creds.c flush.c null.c opdump.c	\
		paths.c pnode.c requests.c subr.c
MAN=		puffs.3 puffs_cc.3 puffs_flush.3 puffs_node.3		\
		puffs_path.3 puffs_req.3
INCS=		puffs.h puffsdump.h
INCSDIR=	/usr/include
LINTFLAGS+=-S -w
#DBG=-O0 -g

.include <bsd.lib.mk>
