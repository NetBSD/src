#	$NetBSD: Makefile,v 1.3 2010/02/28 14:45:47 haad Exp $

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

LIBUMEM_OBJDIR!=  cd ${LIBUMEM_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBUMEM_OBJDIR} -lumem

LDADD+= -lm -lutil -lpthread

.PATH: ${ZFSDIR}/dist/cmd/zfs

.include <bsd.prog.mk>
