#	$NetBSD: lvm2tools.mk,v 1.2 2010/12/23 17:46:54 christos Exp $

.include <bsd.own.mk>

LVM2_SRCDIR=	${NETBSDSRCDIR}/external/gpl2/lvm2
LVM2_DISTDIR=	${NETBSDSRCDIR}/external/gpl2/lvm2/dist

LIBDM_SRCDIR=		${NETBSDSRCDIR}/external/gpl2/lvm2/lib/libdevmapper
LIBDM_DISTDIR=		${NETBSDSRCDIR}/external/gpl2/lvm2/dist/libdm
LIBDM_INCLUDE=		${NETBSDSRCDIR}/external/gpl2/lvm2/dist/include

# root:operator [cb]rw-r-----
CPPFLAGS+=-DDM_DEVICE_UID=0 -DDM_DEVICE_GID=5 -DDM_DEVICE_MODE=0640

#
#LIBDM_OBJDIR.libdevmapper=${LIBDM_SRCDIR}/lib/libdevmapper/
#
#.if !defined(LVM2OBJDIR.liblvm)
#LVM2OBJDIR.liblvm!=	cd ${LVM2_SRCDIR}/lib/liblvm && ${PRINTOBJDIR}
#.MAKEOVERRIDES+=	LVM2OBJDIR.liblvm 
#.endif
#
#LVM2.liblvm=	${LVM2OBJDIR.liblvm}/liblvm.a
