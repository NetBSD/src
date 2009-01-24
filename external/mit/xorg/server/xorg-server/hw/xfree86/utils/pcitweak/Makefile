#	$NetBSD: Makefile,v 1.4 2009/01/24 17:24:52 bouyer Exp $

.include <bsd.own.mk>

PROG=	pcitweak
MAN=	pcitweak.1

.include "../Makefile.utils"

CPPFLAGS+=	-DHAVE_XORG_CONFIG_H \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/dummylib

LDADD=	-L${XORGOSOBJDIR} -lxorgos \
	-L${DUMMYOBJDIR} -ldummy -lm

DPADD=	${LIBXORGOS} ${LIBDUMMY} ${LIBM}

.if ${MACHINE} == "i386"
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
