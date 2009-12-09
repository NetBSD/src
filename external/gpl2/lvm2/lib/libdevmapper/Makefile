#	$NetBSD: Makefile,v 1.4 2009/12/09 00:17:23 haad Exp $

USE_SHLIBDIR=	yes
USE_FORT?=	no
NOLINT=		#defined

.include "../../lvm2tools.mk"

LIB=		devmapper

CPPFLAGS+=	-I${LIBDM_DISTDIR} -I${LIBDM_DISTDIR}/misc -I${LIBDM_DISTDIR}/mm \
		-I${LIBDM_DISTDIR}/datastruct -I${LIBDM_DISTDIR}/ioctl \
		-I${LVM2_DISTDIR}/include -I${LIBDM_SRCDIR} -D__NetBSD__


CPPFLAGS+=	-D__LIB_DEVMAPPER__ -DDM_DEVICE_UID=0 \
		-DDM_DEVICE_GID=0 -DDM_DEVICE_MODE=0600

LDADD+=		-lprop

SRCS+=		bitset.c hash.c list.c libdm-common.c libdm-file.c \
		libdm-deptree.c	libdm-string.c libdm-report.c \
		dbg_malloc.c pool.c matcher.c parse_rx.c ttree.c \
		libdm-nbsd-iface.c libdm_netbsd.c

.ifdef RUMP_ACTION
CPPFLAGS+=      -DRUMP_ACTION -DRUMP_SYS_IOCTL -DRUMP_SYS_CLOSE -DRUMP_SYS_OPEN
LDADD+=         -lrumpdev_disk -lrumpdev_dm -lrumpdev
LDADD+=         -lrumpvfs -lrump -lrumpuser -lpthread
DBG=            -g
.endif

.PATH: ${LIBDM_DISTDIR}/
.PATH: ${LIBDM_DISTDIR}/datastruct
.PATH: ${LIBDM_DISTDIR}/mm
.PATH: ${LIBDM_DISTDIR}/regex
.PATH: ${LIBDM_DISTDIR}/ioctl

.include <bsd.lib.mk>
