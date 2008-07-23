#	$NetBSD: Makefile,v 1.1.2.3 2008/07/23 14:27:03 lukem Exp $

USE_FORT?=	no
NOLINT=		#defined

.include "../../libdevmapper.mk"

PROG=		dmsetup
MAN=		dmsetup.8
MLINKS=		dmsetup.8 dmsetup.1

BINDIR=		/sbin

CPPFLAGS+=      -I${LIBDM_DISTDIR}/lib -I${LIBDM_DISTDIR}/include \
		-I. -I${LIBDM_DISTDIR}/lib/ioctl

CPPFLAGS+=	-D__LIB_DEVMAPPER__ -DDM_DEVICE_UID=0 \
		-DDM_DEVICE_GID=0 -DDM_DEVICE_MODE=0600

LIBDM_OBJDIR!=	cd ${LIBDM_SRCDIR}/lib/libdevmapper && ${PRINTOBJDIR}
LDADD+=		-L${LIBDM_OBJDIR} -ldevmapper
DPADD+=		${LIBDM_OBJDIR}/libdevmapper.a

LDADD+=		-lprop

.PATH:	${LIBDM_DISTDIR}/dmsetup
.PATH:	${LIBDM_DISTDIR}/man

.include <bsd.prog.mk>
