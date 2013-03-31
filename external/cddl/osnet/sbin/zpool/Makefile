#	$NetBSD: Makefile,v 1.4.18.2 2013/03/31 23:06:43 agc Exp $

.include <bsd.own.mk>

.include "../../Makefile.zfs"

PROG=	zpool
MAN=	zpool.8
USETBL=	yes
SRCS=	zpool_main.c zpool_iter.c zpool_util.c zpool_vdev.c

CFLAGS+="-Dprint_timestamp(a)=((void)0)"

DPADD=	${LIBAVL} ${LIBZFS} ${LIBM} ${LIBNVPAIR} ${LIBUUTIL} ${LIBUTIL} ${LIBUMEM}

LIBAVL_OBJDIR!=  cd ${LIBAVL_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBAVL_OBJDIR} -lavl

LIBNVPAIR_OBJDIR!=  cd ${LIBNVPAIR_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBNVPAIR_OBJDIR} -lnvpair

LIBUUTIL_OBJDIR!=  cd ${LIBUUTIL_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBUUTIL_OBJDIR} -luutil

LIBZFS_OBJDIR!=  cd ${LIBZFS_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBZFS_OBJDIR} -lzfs

LIBUMEM_OBJDIR!=  cd ${LIBUMEM_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBUMEM_OBJDIR} -lumem

LIBZPOOL_OBJDIR!=  cd ${LIBZPOOL_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBZPOOL_OBJDIR} -lzpool

LDADD+= -lm -lutil

PROGDPLIBS+=     z		${NETBSDSRCDIR}/lib/libz
PROGDPLIBS+=     pthread	${NETBSDSRCDIR}/lib/libpthread

.PATH: ${ZFSDIR}/dist/cmd/zpool

.include <bsd.prog.mk>
