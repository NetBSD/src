#	$NetBSD: Makefile,v 1.2 2009/10/21 22:31:19 joerg Exp $

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

LDADD+= -lm -lutil

.PATH: ${ZFSDIR}/dist/cmd/zfs

.include <bsd.prog.mk>
