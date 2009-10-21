#	$NetBSD: Makefile,v 1.2 2009/10/21 22:32:20 joerg Exp $

.include "../../Makefile.zfs"

PROG=	zpool
MAN=	zpool.8
USETBL=	yes
SRCS=	zpool_main.c zpool_iter.c zpool_util.c zpool_vdev.c

DPADD=	${LIBAVL} ${LIBZFS} ${LIBM} ${LIBNVPAIR} ${LIBUUTIL} ${LIBUTIL}

LIBAVL_OBJDIR!=  cd ${LIBAVL_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBAVL_OBJDIR} -lavl

LIBNVPAIR_OBJDIR!=  cd ${LIBNVPAIR_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBNVPAIR_OBJDIR} -lnvpair

LIBUUTIL_OBJDIR!=  cd ${LIBUUTIL_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBUUTIL_OBJDIR} -luutil

LIBZFS_OBJDIR!=  cd ${LIBZFS_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBZFS_OBJDIR} -lzfs

LDADD+= -lm -lutil

.PATH: ${ZFSDIR}/dist/cmd/zpool

.include <bsd.prog.mk>
