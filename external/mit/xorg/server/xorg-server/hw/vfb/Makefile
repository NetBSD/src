#	$NetBSD: Makefile,v 1.4.2.2 2008/10/19 22:41:18 haad Exp $

.include <bsd.own.mk>

.include "../../Makefile.Xserver"

PROG=		Xvfb

.PATH:		${X11SRCDIR.xorg-server}/hw/vfb
.PATH:		${X11SRCDIR.xorg-server}/Xext
.PATH:		${X11SRCDIR.xorg-server}/Xi
.PATH:		${X11SRCDIR.xorg-server}/mi
SRCS=		InitInput.c InitOutput.c dpmsstubs.c miinitext.c stubs.c

.PATH:		${X11SRCDIR.xorg-server}/fb
SRCS+=		fbcmap_mi.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xorg-server}/fb \
		-I${X11SRCDIR.xorg-server}/mfb \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${X11SRCDIR.xorg-server}/include \
		-I${X11SRCDIR.xorg-server}/os \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${X11SRCDIR.xorg-server}/render \
		-DHAVE_DIX_CONFIG_H \
		-DNO_HW_ONLY_EXTS \
		-DNO_MODULE_EXTS \
		-UGLXEXT

CPPFLAGS+=	${X11FLAGS.OS_DEFINES} ${X11FLAGS.DIX} \
		-DHAS_SHM -DHAS_MMAP -UXFree86LOADER

.include "../../Makefile.common"

.if ${XSERVER_XFREE86} == "no"
CPPFLAGS+=	-UDDXTIME
.endif

CPPFLAGS.InitInput.c=	${X11FLAGS.EXTENSION} \
			-UGLXEXT
CPPFLAGS.miinitext.c=	${X11FLAGS.EXTENSION} \
			-DNO_HW_ONLY_EXTS -DNO_MODULE_EXTS \
			-UXFree86LOADER \
			-UGLXEXT
CPPFLAGS.dpmsstubs.c=	${X11FLAGS.EXTENSION} \
			-UGLXEXT

COPTS.InitInput.c=	-Wno-error
COPTS.InitOutput.c=	-Wno-error

LDADD+=	${LDADD.dix} \
	${LDADD.fb} \
	${LDADD.mi} \
	${LDADD.xfixes} \
	${LDADD.Xext} \
	${LDADD.config} \
	${LDADD.dbe} \
	${LDADD.XTrap} \
	${LDADD.record} \
	${LDADD.render} \
	${LDADD.randr} \
	${LDADD.damageext} \
	${LDADD.miext/damage} \
	${LDADD.miext/shadow} \
	${LDADD.Xi} \
	${LDADD.xkb} \
	${LDADD.composite} \
	${LDADD.dix/xpstubs} \
	${LDADD.os} \
	-lXfont \
	-lfreetype \
	-lXau \
	-lfontenc \
	${LIBCWRAPPER} \
	-lpixman-1 \
	-lXdmcp \
	-lz \
	-lm

DPADD+=	${DPADD.dix} ${DPADD.fb} ${DPADD.mi} ${DPADD.xfixes} \
	${DPADD.Xext} ${DPADD.config} ${DPADD.dbe} ${DPADD.XTrap} \
	${DPADD.record} ${DPADD.render} ${DPADD.randr} ${DPADD.damageext} \
	${DPADD.miext/damage} ${DPADD.miext/shadow} ${DPADD.Xi} \
	${DPADD.xkb} ${DPADD.composite} ${DPADD.dix/xpstubs} ${DPADD.os} \
	${LIBCWRAPPER} \
	${LIBXFONT} ${LIBFREETYPE} ${LIBXAU} ${LIBFONTENC} \
	${LIBPIXMAN1} ${LIBXDMCP} ${LIBZ} ${LIBM}

.if ${MACHINE_ARCH} == "i386"
LDADD+=	-li386
DPADD+=	${LIBI386}
.endif

.if ${MACHINE_ARCH} == "x86_64"
LDADD+=	-lx86_64
DPADD+=	${LIBX86_64}
.endif

.include <bsd.x11.mk>
.include <bsd.prog.mk>
