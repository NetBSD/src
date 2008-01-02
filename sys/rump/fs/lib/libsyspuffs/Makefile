#	$NetBSD: Makefile,v 1.1.2.2 2008/01/02 21:57:48 bouyer Exp $
#

.include <bsd.own.mk>

LIB=	syspuffs

.PATH:  ${NETBSDSRCDIR}/sys/fs/puffs
.PATH:  ${NETBSDSRCDIR}/sys/dev/putter

SRCS=	puffs_msgif.c puffs_node.c puffs_subr.c puffs_vfsops.c puffs_vnops.c
SRCS+=	putter.c

SRCS+=	puffs_rumpglue.c

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/rump/librump/rumpkern
CPPFLAGS+= -I${NETBSDSRCDIR}/sys/rump/librump/rumpuser
CPPFLAGS+= -DPUFFSDEBUG -DPUTTERDEBUG

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
