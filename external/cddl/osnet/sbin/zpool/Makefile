#	$NetBSD: Makefile,v 1.1 2009/08/07 20:57:56 haad Exp $

.include "../../Makefile.zfs"

PROG=	zpool
MAN=	zpool.8
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
