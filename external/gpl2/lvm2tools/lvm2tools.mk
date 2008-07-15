.include <bsd.own.mk>

LVM2TOOLS_VERSION=   2.02.39

LVM2TOOLS_SRCDIR=    ${NETBSDSRCDIR}/external/gpl2/lvm2tools
LVM2TOOLS_DISTDIR=   ${NETBSDSRCDIR}/external/gpl2/lvm2tools/dist

LVM2TOOLS_PREFIX=    /

LVM2TOOLSOBJDIR.liblvm= ${LVM2TOOLS_SRCDIR}/lib/liblvm


LIBDM_SRCDIR=    ${NETBSDSRCDIR}/external/gpl2/libdevmapper
LIBDM_INCLUDE=    ${LIBDM_SRCDIR}/dist/include
LIBDM_OBJDIR.libdevmapper=    ${LIBDM_SRCDIR}/lib/libdevmapper/

#.if !defined(LVM2TOOLSOBJDIR.liblvm)
#LVM2TOOLSOBJDIR.liblvm   !=    cd ${LVM2TOOLS_SRCDIR}/lib/liblvm & ${PRINTOBJDIR}
#.MAKEOVERRIDES           +=    LVM2TOOLSOBJDIR.liblvm 
#.endif

#LVM2TOOLS.liblvm=    ${LVM2TOOLSOBJDIR.liblvm}/liblvm.a
