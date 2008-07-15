.include <bsd.own.mk>

LIBDM_VERSION=   1.02.27 

LIBDM_SRCDIR=    ${NETBSDSRCDIR}/external/gpl2/libdevmapper
LIBDM_DISTDIR=   ${NETBSDSRCDIR}/external/gpl2/libdevmapper/dist

LIBDM_PREFIX=    /usr

.if !defined(LIBDMOBJDIR.devmapper)
LIBDMOBJDIR.devmapper	!=    cd ${LIBDM_SRCDIR}/lib/libdevmapper && ${PRINTOBJDIR}
.MAKEOVERRIDES		+=    LIBDMOBJDIR.devmapper
.endif

#LIBDMLIB.devmapper	 =    ${LIBDMOBJDIR.devmapper}/libdevmapper.a

