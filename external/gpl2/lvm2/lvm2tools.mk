#	$NetBSD: lvm2tools.mk,v 1.4 2011/03/04 22:48:29 matt Exp $

.include <bsd.own.mk>

LVM2_SRCDIR=	${NETBSDSRCDIR}/external/gpl2/lvm2
LVM2_DISTDIR=	${NETBSDSRCDIR}/external/gpl2/lvm2/dist

LIBDEVMAPPER_SRCDIR=		${LVM2_SRCDIR}/lib/libdevmapper
LIBDEVMAPPER_DISTDIR=		${LVM2_DISTDIR}/libdm
LIBDEVMAPPER_INCLUDE=		${LVM2_DISTDIR}/include

LIBDM_SRCDIR=			${NETBSDSRCDIR}/lib/libdm

# root:operator [cb]rw-r-----
CPPFLAGS+=-DDM_DEVICE_UID=0 -DDM_DEVICE_GID=5 -DDM_DEVICE_MODE=0640 \
	  -DDM_CONTROL_DEVICE_MODE=0660 -DLVM_LOCKDIR_MODE=0770

#
#LIBDEVMAPPER_OBJDIR.libdevmapper=${LIBDEVMAPPER_SRCDIR}/lib/libdevmapper/
#
#.if !defined(LVM2OBJDIR.liblvm)
#LVM2OBJDIR.liblvm!=	cd ${LVM2_SRCDIR}/lib/liblvm && ${PRINTOBJDIR}
#.MAKEOVERRIDES+=	LVM2OBJDIR.liblvm 
#.endif
#
#LVM2.liblvm=	${LVM2OBJDIR.liblvm}/liblvm.a
