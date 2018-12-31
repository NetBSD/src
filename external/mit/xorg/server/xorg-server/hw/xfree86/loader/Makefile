#	$NetBSD: Makefile,v 1.17 2018/12/31 11:14:04 mrg Exp $

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

TOP=	${X11SRCDIR.xorg-server}
XFTOP=	${TOP}/hw/xfree86

LIB=	loader
LOADERDIR=	${XFTOP}/${LIB}
.PATH:	${LOADERDIR}

SRCS=	loader.c loadmod.c sdksyms.c

.if ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64"
#SRCS+=	SparcMulDiv.S
.endif

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/parser \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/vbe \
		-I${X11SRCDIR.xorg-server}/miext/sync \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/int10 \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/ddc \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/i2c \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/modes \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/ramdac \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/dri \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/dri2 \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support/bus \
		-I${X11SRCDIR.xorg-server}/os \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${X11SRCDIR.xorg-server}/xfixes \
		-I${X11SRCDIR.xorg-server}/Xi \
		-I${X11SRCDIR.xorg-server}/include \
		-I${X11SRCDIR.xorg-server}/../include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/dri \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${DESTDIR}${X11INCDIR}/libdrm

CPPFLAGS+=	${X11FLAGS.OS_DEFINES} -DDLOPEN_SUPPORT -DHAS_SHM \
		-DIN_LOADER \
		${x11FLAGS.DIX}

.if ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-DUseMMAP
.endif

CPPFLAGS.loadmod.c=	${X11FLAGS.EXTENSION} -DDEFAULT_MODULE_PATH=\"${X11USRLIBDIR}/modules\"
CPPFLAGS.sdksyms.c=	${X11FLAGS.EXTENSION}

#COPTS.sdksyms.c=		-Wno-error
COPTS.loadmod.c=		-Wno-error

SDKSYMS_CFLAGS=${CFLAGS} ${CPPFLAGS}
# Ugh, pid_t in the header trips the script.
SDKSYMS_CFLAGS+= -Dpid_t=unsigned
sdksyms.c: ${XFTOP}/sdksyms.sh
	CPP=${CPP:Q} AWK=${TOOL_AWK:Q} ${HOST_SH} ${XFTOP}/sdksyms.sh ${TOP} ${SDKSYMS_CFLAGS}
sdksyms.d: sdksyms.c
CLEANFILES+=	sdksyms.c sdksyms.dep

.include <bsd.x11.mk>
.include <bsd.lib.mk>
