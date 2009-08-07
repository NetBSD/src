#	$NetBSD: Makefile,v 1.1 2009/08/07 20:57:56 haad Exp $

.include "../../Makefile.zfs"

PROG=	zfs
MAN=	zfs.8
SRCS=	zfs_main.c zfs_iter.c

DPADD=	${LIBZFS} ${LIBM} ${LIBNVPAIR} ${LIBUUTIL} ${LIBUTIL}

LIBNVPAIR_OBJDIR!=  cd ${LIBNVPAIR_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBNVPAIR_OBJDIR} -lnvpair

LIBUUTIL_OBJDIR!=  cd ${LIBUUTIL_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBUUTIL_OBJDIR} -luutil

LIBZFS_OBJDIR!=  cd ${LIBZFS_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBZFS_OBJDIR} -lzfs

LDADD+= -lm -lutil

.PATH: ${ZFSDIR}/dist/cmd/zfs

.include <bsd.prog.mk>
