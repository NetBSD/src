#	$NetBSD: Makefile,v 1.1.2.3 2008/07/23 13:50:39 lukem Exp $

USE_SHLIBDIR=	yes
USE_FORT?=	no
NOLINT=		#defined

.include "../../libdevmapper.mk"

LIB=		devmapper

CPPFLAGS+=	-I${LIBDM_DISTDIR}/lib -I${LIBDM_DISTDIR}/include \
		-I. -I${LIBDM_DISTDIR}/lib/ioctl


CPPFLAGS+=	-D__LIB_DEVMAPPER__ -DDM_DEVICE_UID=0 \
		-DDM_DEVICE_GID=0 -DDM_DEVICE_MODE=0600

LDADD+=		-lprop

SRCS+=		bitset.c hash.c libdm-common.c libdm-file.c libdm-deptree.c \
		libdm-string.c libdm-report.c dbg_malloc.c pool.c matcher.c \
		parse_rx.c ttree.c libdm-nbsd-iface.c libdm_netbsd.c

.PATH: ${LIBDM_DISTDIR}/lib
.PATH: ${LIBDM_DISTDIR}/lib/datastruct
.PATH: ${LIBDM_DISTDIR}/lib/mm
.PATH: ${LIBDM_DISTDIR}/lib/regex
.PATH: ${LIBDM_DISTDIR}/lib/ioctl

.include <bsd.lib.mk>
