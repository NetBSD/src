#	$NetBSD: Makefile,v 1.3.44.1 2018/06/25 07:25:25 pgoyette Exp $

.include "../../Makefile.zfs"

PROG=	zfs
MAN=	zfs.8
USETBL=	yes
SRCS=	zfs_main.c zfs_iter.c

DPADD=	${LIBZFS} ${LIBM} ${LIBNVPAIR} ${LIBUUTIL} ${LIBUTIL}

LIBNVPAIR_OBJDIR!=  cd ${LIBNVPAIR_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBNVPAIR_OBJDIR} -lnvpair

LIBUUTIL_OBJDIR!=  cd ${LIBUUTIL_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBUUTIL_OBJDIR} -luutil

LIBZFS_OBJDIR!=  cd ${LIBZFS_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBZFS_OBJDIR} -lzfs

LIBZFS_CORE_OBJDIR!=  cd ${LIBZFS_CORE_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBZFS_CORE_OBJDIR} -lzfs_core

LIBUMEM_OBJDIR!=  cd ${LIBUMEM_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBUMEM_OBJDIR} -lumem

LDADD+= -lm -lutil -lpthread

.PATH: ${ZFSDIR}/dist/cmd/zfs

.include <bsd.prog.mk>
