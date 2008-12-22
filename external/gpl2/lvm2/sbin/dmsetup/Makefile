#	$NetBSD: Makefile,v 1.1 2008/12/22 00:57:58 haad Exp $

USE_FORT?=	no
NOLINT=		#defined

.include "../../lvm2tools.mk"

.PATH:  ${LVM2TOOLS_DISTDIR}/man

PROG=		dmsetup

MAN=		dmsetup.8
MLINKS=		dmsetup.8 dmsetup.1

BINDIR=		/sbin

CPPFLAGS+=      -I${LIBDM_DISTDIR}/ -I${LIBDM_DISTDIR}/misc \
		-I. -I${LIBDM_DISTDIR}/ioctl -I${LVM2TOOLS_DISTDIR}/include

CPPFLAGS+=	-D__LIB_DEVMAPPER__ -DDM_DEVICE_UID=0 \
		-DDM_DEVICE_GID=0 -DDM_DEVICE_MODE=0600

LIBDM_OBJDIR!=	cd ${LIBDM_SRCDIR}/lib/libdevmapper && ${PRINTOBJDIR}
LDADD+=		-L${LIBDM_OBJDIR} -ldevmapper
DPADD+=		${LIBDM_OBJDIR}/libdevmapper.a

LDADD+=		-lprop

.PATH:	${LVM2TOOLS_DISTDIR}/tools

.include <bsd.prog.mk>
