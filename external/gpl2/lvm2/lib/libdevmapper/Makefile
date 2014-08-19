#	$NetBSD: Makefile,v 1.8.10.1 2014/08/19 23:52:23 tls Exp $

USE_SHLIBDIR=	yes
USE_FORT?=	no
NOLINT=		#defined

.include "../../lvm2tools.mk"

LIB=		devmapper

CWARNFLAGS.clang+=	-Wno-comment -Wno-parentheses -Wno-switch

CPPFLAGS+=	-I${LIBDEVMAPPER_DISTDIR} \
		-I${LIBDEVMAPPER_DISTDIR}/misc \
		-I${LIBDEVMAPPER_DISTDIR}/mm \
		-I${LIBDEVMAPPER_DISTDIR}/datastruct \
		-I${LIBDEVMAPPER_DISTDIR}/ioctl \
		-I${LVM2_DISTDIR}/include \
		-I${LIBDEVMAPPER_SRCDIR} \
		-D__NetBSD__

CPPFLAGS+=	-D__LIB_DEVMAPPER__ 

LIBDPLIBS+=	dm ${NETBSDSRCDIR}/lib/libdm

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

.PATH: ${LIBDEVMAPPER_DISTDIR}/
.PATH: ${LIBDEVMAPPER_DISTDIR}/datastruct
.PATH: ${LIBDEVMAPPER_DISTDIR}/mm
.PATH: ${LIBDEVMAPPER_DISTDIR}/regex
.PATH: ${LIBDEVMAPPER_DISTDIR}/ioctl

.include <bsd.lib.mk>
