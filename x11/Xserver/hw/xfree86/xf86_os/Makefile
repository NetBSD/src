#	$NetBSD: Makefile,v 1.1 2003/12/22 13:27:48 rtr Exp $

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	xf86_os

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/bsd
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/shared
SRCS.bsd=	bsd_init.c bsd_io.c bsd_VTsw.c libc_wrapper.c std_kbdEv.c \
		posix_tty.c bsd_mouse.c stdPci.c vidmem.c sigio.c \
		kmod_noop.c bsd_kbd.c bsd_KbdMap.c at_scancode.c

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/linux
.if ${MACHINE} == "i386"
SRCS.bsd+=	i386_video.c stdResource.c bsd_apm.c lnx_agp.c ioperm_noop.c
.for _FN in ${SRCS.bsd}
CPPFLAGS.${_FN}=	-DPCCONS_SUPPORT -DPCVT_SUPPORT -DWSCONS_SUPPORT \
			-DHAS_APERTURE_DRV -DUSE_I386_IOPL -DUSESTDRES \
			-DHAS_MTRR_BUILTIN -DHAVE_SYSV_IPC
.endfor
.endif

.if ${MACHINE} == "macppc"
SRCS.bsd+=	ppc_video.c bsdResource.c pm_noop.c lnx_agp.c ioperm_noop.c
.for _FN in ${SRCS.bsd}
CPPFLAGS.${_FN}=	-DWSCONS_SUPPORT -DUSESTDRES -DHAVE_SYSV_IPC
.endfor
.endif

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/bus
SRCS.bus=	Pci.c

.if ${MACHINE} == "i386"
SRCS.bus+=	ix86Pci.c
.endif

.if ${MACHINE} == "macppc"
SRCS.bus+=	netbsdPci.c
.endif


.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/misc
SRCS.misc=	xf86_Util.c Delay.c xf86_IlHack.c

.if ${MACHINE} == "i386"
SRCS.misc+=	BUSmemcpy.S IODelay.S SlowBcopy.S
CPPFLAGS.BUSmemcpy.S=	-DUSE_GAS
CPPFLAGS.IODelay.S=	-DUSE_GAS
CPPFLAGS.SlowBcopy.S=	-DUSE_GAS
.endif

.if ${MACHINE} == "macppc"
SRCS.misc+=	CBUSmemcpy.c CIODelay.c CSlowBcopy.c
.endif

# XXX namespace collision so avoid it, only has to be done for non-i386.
BUILDSYMLINKS=	BUSmemcpy.c CBUSmemcpy.c \
		IODelay.c CIODelay.c \
		SlowBcopy.c CSlowBcopy.c

SRCS=	${SRCS.bsd} ${SRCS.bus} ${SRCS.misc}

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/bsd \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/bus \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/misc \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/shared \
		-I${XSERVERINCDIR}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
