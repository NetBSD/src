#	$NetBSD: Makefile,v 1.8 2016/01/05 13:07:47 christos Exp $

.include <bsd.own.mk>

LIB=	pciaccess
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include

XORG_MACHINE_ARCH?=${MACHINE_ARCH}

SRCS=	common_bridge.c \
	common_io.c \
	common_iterator.c \
	common_init.c \
	common_interface.c \
	common_capability.c \
	common_device_name.c \
	common_map.c \
	common_vgaarb_stub.c \
	netbsd_pci.c

CPPFLAGS+=	${X11FLAGS} -I${X11SRCDIR.${LIB}}/include -DHAVE_ZLIB \
		-DHAVE_STRING_H -DHAVE_STDINT_H -DHAVE_INTTYPES_H \
		-DPCIIDS_PATH=\"${X11ROOTDIR}/share/pciids\"

COPTS.common_init.c=	-Wno-error

INCS=	pciaccess.h
INCSDIR=${X11INCDIR}

PKGDIST=	${LIB}

LIBDPLIBS+=	pci	${NETBSDSRCDIR}/lib/libpci \
		z	${NETBSDSRCDIR}/lib/libz

.if ${XORG_MACHINE_ARCH} == "i386" || ${XORG_MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-DHAVE_MTRR
.endif

.if ${XORG_MACHINE_ARCH} == "i386"
LIBDPLIBS+=	i386 ${NETBSDSRCDIR}/lib/libarch
.endif

.if ${XORG_MACHINE_ARCH} == "x86_64"
LIBDPLIBS+=	x86_64 ${NETBSDSRCDIR}/lib/libarch
.endif

.include <bsd.x11.mk>
.include <bsd.lib.mk>
