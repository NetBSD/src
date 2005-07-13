#	$NetBSD: Makefile,v 1.8 2005/07/13 02:04:01 macallan Exp $

.include <bsd.sys.mk> # for HOST_SH

.include "../../../Makefile.serverlib"

LIB=	xf86

# XXX xf86Init.c & xf86IniExt.c should not be linked into libxf86.a
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common
SRCS=	xf86Configure.c xf86Bus.c xf86isaBus.c xf86pciBus.c \
	xf86fbBus.c xf86noBus.c xf86IniExt.c xf86Config.c \
	xf86AutoConfig.c xf86Cursor.c xf86DGA.c xf86DPMS.c \
	xf86DefModeSet.c xf86DoProbe.c xf86DoScanPci.c xf86Events.c \
	xf86Globals.c xf86Helper.c xf86Init.c xf86Io.c xf86MiscExt.c \
	xf86Mode.c xf86Option.c xf86VidMode.c xf86fbman.c xf86xv.c \
	xf86xvmc.c xf86cmap.c xf86PM.c xf86Versions.c xf86Debug.c \
	xf86KbdBSD.c xf86Xinput.c xisb.c xf86XKB.c xf86RandR.c

.if ${MACHINE} == "sparc64"
SRCS+=	xf86sbusBus.c
CPPFLAGS+=-DUSE_MODULAR_KBD
.endif

.if ${MACHINE} == "macppc"
CPPFLAGS+=-DUSE_MODULAR_KBD
.endif

# Rebuild xf86Build.h on every make
#
CLEANFILES+=	xf86Build.h
xf86Build.h: .PHONY
	${_MKTARGET_CREATE}
	echo "#define BUILD_DATE `date +%Y%m%d`" > ${.TARGET}

# We don't use  DPSRCS+=xf86Build.h  because we don't want the entire
# library to be rebuilt each time
#
xf86Init.c xf86Init.o: xf86Build.h

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/bus \
                -I${X11SRCDIR.xc}/programs/Xserver/mfb \
                -I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/os \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
                -I${X11SRCDIR.xc}/programs/Xserver/Xi \
                -I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ddc \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/i2c \
                -I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86 \
                -I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/parser \
                -I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/loader \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/vgahw \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/rac \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/scanpci \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/vbe \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/int10 \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${X11SRCDIR.xc}/programs/Xserver/randr \
		-I. \
		-I${XSERVERINCDIR}

CPPFLAGS+=	${X11FLAGS.EXTENSION} ${X11FLAGS.OS_DEFINES} \
		-DXFree86LOADER -DWSCONS_SUPPORT

.if ${MACHINE} == "cats"
CPPFLAGS+=	-DPCVT_SUPPORT
.endif

.if ${MACHINE} == "i386"
CPPFLAGS+=	-DPCCONS_SUPPORT -DPCVT_SUPPORT
.endif

BUILDSYMLINKS=	xf86DefModes.c xf86DefModeSet.c \
		../../../mi/miinitext.c xf86IniExt.c 

OS_RELEASE!=	${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh

DRIVERS=	mga glint nv tga s3 s3virge sis rendition neomagic \
		i740 tdfx savage cirrus vmware tseng trident chips \
		apm i128
.if ${MACHINE_ARCH} != "x86_64"
DRIVERS+=	nsc
.endif
DRIVERS+=	ati i810 ark cyrix siliconmotion via fbdev vesa vga \
		dummy

IDRIVERS=	mouse keyboard acecad calcomp citron digitaledge \
		dmc dynapro elographics tek4957 microtouch \
		mutouch penmount spaceorb summa wacom void \
		magellan hyperpen jamstudio fpit palmax

CPPFLAGS.xf86Bus.c=		-DBUILDERADDR='"xfree86@xfree86.org"'
CPPFLAGS.xf86Init.c=		-DOSNAME='"NetBSD/${MACHINE} ${OS_RELEASE} -"' \
				-DXORG_RELEASE='"Release 6.6"' \
				-DBUILDERADDR='"xfree86@xfree86.org"' \
				-DOSVENDOR='" The NetBSD Foundation, Inc."'
CPPFLAGS.xf86Globals.c=		-DDEFAULT_LOGPREFIX='"/var/log/XFree86."' \
				-DDEFAULT_MODULE_PATH='"${X11USRLIBDIR}/modules"'
CPPFLAGS.xf86Configure.c=	-DXF86CONFIGFILE='"XF86Config"'
CPPFLAGS.xf86Config.c=		-DDRIVERS='"${DRIVERS}"' \
				-DIDRIVERS='"${IDRIVERS}"' \
				-DPROJECTROOT='"${X11ROOTDIR}"'
CPPFLAGS.xf86AutoConfig.c=	-DPROJECTROOT='"${X11ROOTDIR}"' \
				-DBINDIR='"${X11BINDIR}"' \
				-DDRIVERS='"${DRIVERS}"' \
				-DIDRIVERS='"${IDRIVERS}"'

.include <bsd.x11.mk>
.include <bsd.lib.mk>
