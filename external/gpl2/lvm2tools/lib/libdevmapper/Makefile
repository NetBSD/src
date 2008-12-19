#	$NetBSD: Makefile,v 1.2 2008/12/19 15:24:18 haad Exp $

USE_SHLIBDIR=	yes
USE_FORT?=	no
NOLINT=		#defined

.include "../../lvm2tools.mk"

LIB=		devmapper

CFLAGS+= -fno-inline

CPPFLAGS+=	-I${LIBDM_SRCDIR} -I${LIBDM_SRCDIR}/misc -I${LIBDM_SRCDIR}/mm \
		-I${LIBDM_SRCDIR}/datastruct -I${LIBDM_SRCDIR}/ioctl \
		-I${LVM2TOOLS_DISTDIR}/include -I. -D__NetBSD__


CPPFLAGS+=	-D__LIB_DEVMAPPER__ -DDM_DEVICE_UID=0 \
		-DDM_DEVICE_GID=0 -DDM_DEVICE_MODE=0600

LDADD+=		-lprop

SRCS+=		bitset.c hash.c list.c libdm-common.c libdm-file.c \
		libdm-deptree.c	libdm-string.c libdm-report.c \
		dbg_malloc.c pool.c matcher.c parse_rx.c ttree.c \
		libdm-nbsd-iface.c libdm_netbsd.c

.PATH: ${LIBDM_SRCDIR}/
.PATH: ${LIBDM_SRCDIR}/datastruct
.PATH: ${LIBDM_SRCDIR}/mm
.PATH: ${LIBDM_SRCDIR}/regex
.PATH: ${LIBDM_SRCDIR}/ioctl

.include <bsd.lib.mk>
