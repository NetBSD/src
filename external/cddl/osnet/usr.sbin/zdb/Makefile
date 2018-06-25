#	$NetBSD: Makefile,v 1.3.44.1 2018/06/25 07:25:27 pgoyette Exp $

.include "../../Makefile.zfs"

PROG=	zdb
MAN=	zdb.8
USETBL=	yes
SRCS=	zdb.c zdb_il.c

DPADD+=	${LIBM} ${LIBPTHREAD}

LIBAVL_OBJDIR!=  cd ${LIBAVL_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBAVL_OBJDIR} -lavl

LIBNVPAIR_OBJDIR!=  cd ${LIBNVPAIR_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBNVPAIR_OBJDIR} -lnvpair

LIBUMEM_OBJDIR!=  cd ${LIBUMEM_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBUMEM_OBJDIR} -lumem

LIBUUTIL_OBJDIR!=  cd ${LIBUUTIL_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBUUTIL_OBJDIR} -luutil

LIBZFS_OBJDIR!=  cd ${LIBZFS_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBZFS_OBJDIR} -lzfs

LIBZFS_CORE_OBJDIR!=  cd ${LIBZFS_CORE_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBZFS_CORE_OBJDIR} -lzfs_core

LIBZPOOL_OBJDIR!=  cd ${LIBZPOOL_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBZPOOL_OBJDIR} -lzpool

LDADD+= 	-lm -lpthread

COPTS.zdb_il.c += -Wno-stack-protector

.PATH:	${ZFSDIR}/dist/cmd/zdb

.include <bsd.prog.mk>
