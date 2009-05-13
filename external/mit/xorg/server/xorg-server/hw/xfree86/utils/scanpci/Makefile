#	$NetBSD: Makefile,v 1.5.2.1 2009/05/13 18:53:29 jym Exp $

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
	-Wl,-rpath,${X11USRLIBDIR}/modules \
	-L${XORGOSOBJDIR} -lxorgos \
	-L${DUMMYOBJDIR} -ldummy -lm

DPADD=	${LIBSCANPCI} ${LIBXORGOS} ${LIBDUMMY} ${LIBM}

.if ${MACHINE_ARCH} == "i386"
LDADD+=	-li386
DPADD+=	${LIBI386}
.elif ${MACHINE_ARCH} == "x86_64"
LDADD+=	-lx86_64
DPADD+=	${LIBX86_64}
.elif ${MACHINE_ARCH} == "alpha"
LDADD+=	-lalpha
DPADD+=	${LIBALPHA}
.endif

.PATH: ${X11SRCDIR.xorg-server}/hw/xfree86/utils/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
