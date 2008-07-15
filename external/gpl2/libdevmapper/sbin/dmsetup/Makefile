USE_FORT ?= no
MKLINT = no

.include "../../libdevmapper.mk"

PROG=		dmsetup

BINDIR=		/sbin

CPPFLAGS+=      -I${LIBDM_DISTDIR}/lib -I${LIBDM_DISTDIR}/include\
		-I. -I${LIBDM_DISTDIR}/lib/ioctl

CPPFLAGS+= 	-D__LIB_DEVMAPPER__ -DDM_DEVICE_UID=0 \
		-DDM_DEVICE_GID=0 -DDM_DEVICE_MODE=0600

CFLAGS+= -I${LIBDM_DISTDIR}/lib -I${LIBDM_DISTDIR}/include\
	 -I. -I${LIBDM_DISTDIR}/lib/ioctl

CFLAGS+= -DDEVICE_UID=0 -DDEVICE_GID=0

LDADD+= -L${LIBDMOBJDIR.devmapper} -ldevmapper
DPADD+= ${LIBDMLIB.devmapper}

LDADD+= -lprop

.PATH:  ${LIBDM_DISTDIR}/dmsetup
.PATH:  ${LIBDM_DISTDIR}/man

MAN= dmsetup.8

MLINKS= dmsetup.8 dmsetup.1

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
