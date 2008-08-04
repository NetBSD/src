#	$NetBSD: Makefile,v 1.1 2008/08/04 01:03:34 mrg Exp $

.include <bsd.own.mk>

PROG=	ioport
# XXX NOMAN no workie?
MKMAN=	no

.include "../Makefile.utils"

CPPFLAGS+=	-DHAVE_XORG_CONFIG_H \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/dummylib


LDADD=	-L${XF86OSOBJDIR} -lxorgos \
	-L${DUMMYOBJDIR} -ldummy -lm

.if ${MACHINE} == "i386"
LDADD+=	-li386
DPADD+=	${LIBI386}
.else
LDADD+=	-lx86_64
DPADD+=	${LIBX86_64}
.endif

DPADD=	${LIBXORGOS} ${LIBDUMMY} ${LIBM}

LINKS=  ${X11BINDIR}/ioport ${X11BINDIR}/inb \
	${X11BINDIR}/ioport ${X11BINDIR}/inw \
	${X11BINDIR}/ioport ${X11BINDIR}/inl \
	${X11BINDIR}/ioport ${X11BINDIR}/outb \
	${X11BINDIR}/ioport ${X11BINDIR}/outw \
	${X11BINDIR}/ioport ${X11BINDIR}/outl 

.PATH: ${X11SRCDIR.xorg-server}/hw/xfree86/utils/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
