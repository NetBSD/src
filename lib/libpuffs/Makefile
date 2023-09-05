#	$NetBSD: Makefile,v 1.30 2023/09/05 00:12:14 mrg Exp $
#

.include <bsd.own.mk>

USE_FORT?=	yes	# data-driven bugs?

WARNS?=		5

LIB=		puffs

SRCS=		puffs.c callcontext.c creds.c dispatcher.c flush.c	\
		framebuf.c null.c opdump.c paths.c pnode.c requests.c	\
		subr.c suspend.c
MAN=		puffs.3 puffs_cc.3 puffs_cred.3 puffs_flush.3		\
		puffs_framebuf.3 puffs_node.3 puffs_ops.3 puffs_path.3
INCS=		puffs.h puffsdump.h
INCSDIR=	/usr/include
LINTFLAGS+=-S -w
LINTFLAGS+=	-X 351		# extern declaration outside header
LINTFLAGS+=	-X 352		# nested extern declaration

# XXXGCC12 - accesses struct puffs_cc* via stack pointers, returns the
# address to caller
COPTS.callcontext.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 12:? -Wno-dangling-pointer :}

.include <bsd.lib.mk>
