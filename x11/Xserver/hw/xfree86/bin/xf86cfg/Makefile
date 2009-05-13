#	$NetBSD: Makefile,v 1.6.26.1 2009/05/13 19:20:47 jym Exp $

.include <bsd.own.mk>

PROG=	xf86cfg

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf86cfg
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf86config	# for Cards
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86		# for Options

# XXX only if loadable LDSRCS  = libc_wrapper.c loader.c loadmod.c
SRCS=	accessx.c card-cfg.c cards.c config.c expert.c help.c interface.c \
	keyboard-cfg.c libc_wrapper.c loader.c loadmod.c monitor-cfg.c \
	mouse-cfg.c options.c screen-cfg.c screen.c startx.c stubs.c \
	vidmode.c xf86config.c

DPSRCS+=xf86PciIds.h

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}
SCANPCIINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/hw/xfree86/bin/xf86cfg && ${PRINTOBJDIR}
XF86OBJDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/hw/xfree86/xf86config && ${PRINTOBJDIR}
XLOADEROBJDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/hw/xfree86/xloader && ${PRINTOBJDIR}
DUMMYOBJDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/hw/xfree86/dummy && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/scanpci \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/loader \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${SCANPCIINCDIR} \
		-I${XSERVERINCDIR}

# XXX only if loadable -DUSE_MODULES
CPPFLAGS+=	-DXF86CONFIG=\"XF86Config\" -DUSE_MODULES \
		-DPROJECT_ROOT=\"/usr/X11R6\" \
		-DXF86CONFIGDIR=\"/etc/X11\" -DWSCONS_SUPPORT

.if ${MACHINE} == "cats"
CPPFLAGS+=	-DPCVT_SUPPORT
.endif

.if ${MACHINE_ARCH} == "i386"
CPPFLAGS+=	-DPCCONS_SUPPORT -DPCVT_SUPPORT
.endif


LDADD=	-lxkbui -lxkbfile -L${XF86OBJDIR} -lxf86config -lXxf86misc \
	-lXxf86vm -lXaw -lXmu -lXt -lSM -lICE -lXext -lX11 -lXt -lSM \
	-lICE -lXpm -L${XLOADEROBJDIR} -lxloader -L${DUMMYOBJDIR} \
	-ldummy -lXext -lX11 -lm

DPADD=	${LIBXKBUI} ${LIBXKBFILE} ${LIBXF86CONFIG} ${LIBXXF86MISC} \
	${LIBXXF86VM} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXEXT} ${LIBX11} ${LIBXPM} ${LIBXLOADER} ${LIBDUMMY} ${LIBM}

BUILDSYMLINKS=	../os-support/shared/libc_wrapper.c libc_wrapper.c \
		../scanpci/xf86PciStdIds.h xf86PciIds.h

CPPFLAGS.libc_wrapper.c=	-DSELF_CONTAINED_WRAPPER

APPDEFS=XF86Cfg.ad

FILES.bitmap=	card.xbm keyboard.xbm monitor.xbm mouse.xbm left.xbm \
		right.xbm up.xbm down.xbm wider.xbm narrower.xbm \
		shorter.xbm taller.xbm

FILES.pixmap=	card.xpm computer.xpm keyboard.xpm monitor.xpm mouse.xpm

# XXX Cards Options are both used by xf86cfg as well as other things
#     so this may not be the appropriate place to install from.
FILES=	${FILES.bitmap} ${FILES.pixmap} Cards Options

.for _FN in ${FILES.bitmap}
FILESDIR_${_FN}=	${X11INCDIR}/X11/bitmaps
.endfor

.for _FN in ${FILES.pixmap}
FILESDIR_${_FN}=	${X11INCDIR}/X11/pixmaps
.endfor

FILESDIR_Cards=		${X11USRLIBDIR}/X11
FILESDIR_Options=	${X11USRLIBDIR}/X11

.include <bsd.x11.mk>

# need to do this here, so that X11FLAGS.LOADABLE is available
.if (${MACHINE_ARCH} == "arm" || ${MACHINE_ARCH} == "armeb") \
        && defined(X11FLAGS.LOADABLE)
LDADD+= -larm
DPADD+= ${LIBARM}
.endif

.include <bsd.prog.mk>
