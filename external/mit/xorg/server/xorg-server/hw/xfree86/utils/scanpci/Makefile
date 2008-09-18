#	$NetBSD: Makefile,v 1.2.2.2 2008/09/18 05:18:47 wrstuden Exp $

.include <bsd.own.mk>

PROG=	scanpci
MAN=	scanpci.1

.include "../Makefile.utils"

CPPFLAGS+=	-DHAVE_XORG_CONFIG_H \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/scanpci \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/dummylib

LDADD=	-L${SCANPCIOBJDIR} -lscanpci \
	-L${XORGOSOBJDIR} -lxorgos \
	-L${DUMMYOBJDIR} -ldummy -lm

DPADD=	${LIBSCANPCI} ${LIBXORGOS} ${LIBDUMMY} ${LIBM}

.if ${MACHINE} == "i386"
LDADD+=	-li386
DPADD+=	${LIBI386}
.elif ${MACHINE_ARCH} == "x86_64"
LDADD+=	-lx86_64
DPADD+=	${LIBX86_64}
.endif

.PATH: ${X11SRCDIR.xorg-server}/hw/xfree86/utils/${PROG}

# XXX
scanpci.1: scanpci.man.pre
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
