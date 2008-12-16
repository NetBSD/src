#	$NetBSD: lvm2tools.mk,v 1.1.2.3 2008/12/16 00:43:53 haad Exp $

.include <bsd.own.mk>

LVM2TOOLS_SRCDIR=	${NETBSDSRCDIR}/external/gpl2/lvm2tools
LVM2TOOLS_DISTDIR=	${NETBSDSRCDIR}/external/gpl2/lvm2tools/dist

LVM2TOOLS_PREFIX=	/

LIBDM_SRCDIR=		${NETBSDSRCDIR}/external/gpl2/lvm2tools/dist/libdm
LIBDM_INCLUDE=		${NETBSDSRCDIR}/external/gpl2/lvm2tools/dist/include

#
#LIBDM_OBJDIR.libdevmapper=${LIBDM_SRCDIR}/lib/libdevmapper/
#
#.if !defined(LVM2TOOLSOBJDIR.liblvm)
#LVM2TOOLSOBJDIR.liblvm!=	cd ${LVM2TOOLS_SRCDIR}/lib/liblvm && ${PRINTOBJDIR}
#.MAKEOVERRIDES+=	LVM2TOOLSOBJDIR.liblvm 
#.endif
#
#LVM2TOOLS.liblvm=	${LVM2TOOLSOBJDIR.liblvm}/liblvm.a
