#	$NetBSD: Makefile,v 1.16 2007/05/05 15:48:18 pooka Exp $
#

.include <bsd.own.mk>

WARNS=		4

LIB=		puffs
SRCS=		puffs.c callcontext.c creds.c flush.c framebuf.c null.c	\
		opdump.c paths.c pnode.c requests.c subr.c suspend.c
MAN=		puffs.3 puffs_cc.3 puffs_cred.3 puffs_flush.3		\
		puffs_node.3 puffs_ops.3 puffs_path.3 puffs_req.3	\
		puffs_suspend.3
INCS=		puffs.h puffsdump.h
INCSDIR=	/usr/include
LINTFLAGS+=-S -w
#DBG=-O0 -g

.include <bsd.lib.mk>
