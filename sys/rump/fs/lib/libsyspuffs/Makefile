#	$NetBSD: Makefile,v 1.2.4.2 2008/01/21 09:47:40 yamt Exp $
#

.include <bsd.own.mk>

LIB=	syspuffs

.PATH:  ${NETBSDSRCDIR}/sys/fs/puffs
.PATH:  ${NETBSDSRCDIR}/sys/dev/putter

SRCS=	puffs_msgif.c puffs_node.c puffs_subr.c puffs_vfsops.c puffs_vnops.c
SRCS+=	putter.c

SRCS+=	puffs_rumpglue.c

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/rump/librump/rumpkern -I${LIBRUMPDIR}
CPPFLAGS+= -I${NETBSDSRCDIR}/sys/rump/librump/rumpuser
CPPFLAGS+= -DPUFFSDEBUG -DPUTTERDEBUG

.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
