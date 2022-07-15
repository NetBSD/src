#	$NetBSD: Makefile,v 1.51 2022/07/15 04:18:55 mrg Exp $

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	xorgos

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/os-support
.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/os-support/bsd
.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/os-support/shared
SRCS.bsd=	\
		bsd_VTsw.c bsd_bell.c bsd_init.c bsd_pci.c \
		kmod_noop.c pm_noop.c \
		posix_tty.c sigio.c vidmem.c

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/os-support/linux
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SRCS.bsd+=	i386_video.c lnx_agp.c
.for _FN in ${SRCS.bsd}
.if ${MACHINE_ARCH} == "i386"
CPPFLAGS.${_FN}+=	-DPCCONS_SUPPORT -DHAS_APERTURE_DRV
CPPFLAGS.${_FN}+=	-DUSE_I386_IOPL
.else
CPPFLAGS.${_FN}+=	-DUSE_AMD64_IOPL
.endif
CPPFLAGS.${_FN}+=	-DPCVT_SUPPORT -DWSCONS_SUPPORT \
			-DUSESTDRES -DHAVE_SYSV_IPC \
			-DHAS_MTRR_BUILTIN
.endfor
.endif

.if ${MACHINE_CPU} == "powerpc" || ${MACHINE} == "hppa"
SRCS.bsd+=	ppc_video.c
.  if ${MACHINE} != "evbppc"
SRCS.bsd+=	lnx_agp.c
.  else
SRCS.bsd+=	agp_noop.c
.  endif
.for _FN in ${SRCS.bsd}
CPPFLAGS.${_FN}=	-DWSCONS_SUPPORT -DPCVT_SUPPORT -DUSESTDRES -DHAVE_SYSV_IPC
.endfor
.endif

.if ${MACHINE_ARCH} == "alpha"
SRCS.bsd+=	alpha_video.c agp_noop.c
.for _FN in ${SRCS.bsd}
CPPFLAGS.${_FN}=	-DWSCONS_SUPPORT -DUSESTDRES -DHAVE_SYSV_IPC -DUSE_ALPHA_PIO
.endfor
.endif

.if ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64"
SRCS.bsd+=	ppc_video.c agp_noop.c
SRCS.bus+=	netbsdSbus.c
.for _FN in ${SRCS.bsd}
CPPFLAGS.${_FN}=	-DWSCONS_SUPPORT -DPCVT_SUPPORT -DUSESTDRES -DHAVE_SYSV_IPC
.endfor
.endif

.if ${MACHINE} == "ews4800mips" || \
    ${MACHINE} == "hpcmips" || \
    ${MACHINE} == "newsmips" || \
    ${MACHINE} == "sgimips" || \
    ${MACHINE} == "evbmips"
SRCS.bsd+=	ppc_video.c agp_noop.c
.for _FN in ${SRCS.bsd}
CPPFLAGS.${_FN}=	-DWSCONS_SUPPORT -DPCVT_SUPPORT  -DUSESTDRES -DHAVE_SYSV_IPC
.endfor
.endif

.if ${MACHINE} == "cats" || \
    ${MACHINE} == "evbarm" || \
    ${MACHINE} == "hpcarm" || \
    ${MACHINE} == "iyonix" || \
    ${MACHINE} == "shark" || \
    ${MACHINE} == "netwinder" || \
    ${MACHINE} == "zaurus"
SRCS.bsd+=      ppc_video.c agp_noop.c
.for _FN in ${SRCS.bsd}
CPPFLAGS.${_FN}=        -DWSCONS_SUPPORT -DPCVT_SUPPORT -DUSESTDRES -DHAVE_SYSV_IPC
.endfor
.endif

.if ${MACHINE} == "shark"
CPPFLAGS.ppc_video.c += -DPCI_MAGIC_IO_RANGE=0xf0000000
.endif

.if ${MACHINE} == "dreamcast" || \
    ${MACHINE} == "hpcsh"
SRCS.bsd+=	ppc_video.c agp_noop.c
.for _FN in ${SRCS.bsd}
CPPFLAGS.${_FN}=	-DWSCONS_SUPPORT -DUSESTDRES -DHAVE_SYSV_IPC
.endfor
.endif

.if ${MACHINE} == "amiga" || \
    ${MACHINE} == "hp300" || \
    ${MACHINE} == "luna68k" || \
    ${MACHINE} == "mac68k"
SRCS.bsd+=	ppc_video.c agp_noop.c
.for _FN in ${SRCS.bsd}
CPPFLAGS.${_FN}=	-DWSCONS_SUPPORT -DUSESTDRES -DHAVE_SYSV_IPC
.endfor
.endif

.if ${MACHINE} == "pmax" || \
    ${MACHINE} == "vax"
SRCS.bsd+=	ppc_video.c agp_noop.c
.for _FN in ${SRCS.bsd}
CPPFLAGS.${_FN}=	-DWSCONS_SUPPORT -DUSESTDRES -DHAVE_SYSV_IPC
.endfor
.endif

# some platforms will probably need ioperm_noop.c

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/os-support/bus
SRCS.bus+=	Pci.c

.if ${MACHINE} == "macppc" || ${MACHINE} == "cats" || \
    ${MACHINE} == "sgimips" || ${MACHINE} == "shark" || \
    ${MACHINE} == "netwinder" || ${MACHINE_ARCH} == "sparc" || \
    ${MACHINE_ARCH} == "sparc64" || ${MACHINE_ARCH} == "alpha" || \
    ${MACHINE} == "prep" || ${MACHINE} == "bebox" || \
    ${MACHINE} == "ofppc" || ${MACHINE} == "ibmnws"
#SRCS.bus+=	netbsdPci.c
.endif

.if ${MACHINE} == "sparc" || ${MACHINE} == "sparc64" || \
    ${MACHINE_ARCH} == "powerpc" || ${MACHINE} == "netwinder"
#CPPFLAGS.netbsdPci.c+= -DPCI_VIA_TTYE0
.endif

.if     ${MACHINE} == "macppc" || ${MACHINE} == "cats" || \
	${MACHINE} == "sgimips" || ${MACHINE_ARCH} == "sparc64" || \
	${MACHINE_ARCH} == "sparc" || ${MACHINE} == "netwinder" || \
	${MACHINE} == "shark" || ${MACHINE} == "ofppc" || \
	${MACHINE_ARCH} == "alpha" || ${MACHINE} == "prep" || \
	${MACHINE} == "bebox" || ${MACHINE} == "ibmnws"
#SRCS.misc+=     CBUSmemcpy.c CIODelay.c CSlowBcopy.c
.endif

# All platforms get this.
SRCS.bus+=	nobus.c

.PATH:		${X11SRCDIR.xorg-server}/hw/xfree86/os-support/misc
SRCS.misc+=     SlowBcopy.c

SRCS=	${SRCS.bsd} ${SRCS.bus} ${SRCS.misc}

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support/bsd \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support/bus \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support/misc \
		-I${X11SRCDIR.xorg-server}/include \
		-I${X11SRCDIR.xorg-server}/miext/damage \
		-I${X11SRCDIR.xorg-server}/render \
		-I${X11SRCDIR.xorg-server}/present \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support/shared \
		-I${X11SRCDIR.xorg-server}/../include
.if ${X11DRI} != "no"
CPPFLAGS+=	${X11FLAGS.DRI}
.endif

CWARNFLAGS.clang+=	-Wno-error=empty-body

.include <bsd.x11.mk>
.include <bsd.lib.mk>
