#	$NetBSD: Makefile,v 1.4 2004/01/06 06:18:14 rtr Exp $

.include <bsd.own.mk>

PROG=	Xsun
BINMODE=4711

MAN=		Xsun.1
MLINKS=		Xsun.1 Xsun24.1 Xsun.1 XsunMono.1

.include "../Makefile.Xserver"

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/sun

LDADD=	${LIBDIR.hw/sun}/sunInit.o ${LDADD.hw/sun} ${LDADD.dix} ${LDADD.os} \
	${LDADD.cfb} ${LDADD.mfb} ${LDADD.dix/xpstubs} ${LDADD.mi} \
	${LDADD.Xext/ext} ${LDADD.xkb}

.if ${MACHINE} == "sparc64"
LDADD+=	${LDADD.xinput}
DPADD=	${DPADD.xinput}
.endif

LDADD+=	${LDADD.lbx} -llbxutil ${LDADD.dbe} ${LDADD.record} \
	${LDADD.xtrap} ${LDADD.GL/glx} ${LDADD.GL/GLcore} ${LDADD.randr} \
	${LDADD.render} -lXfont -lfreetype ${LDADD.dix/xpstubs} \
	-lfreetype -lXau -lXdmcp -lz -lm

DPADD+=	${LIBDIR.hw/sun}/sunInit.o ${DPADD.hw/sun} ${DPADD.dix} ${DPADD.os} \
	${DPADD.cfb} ${DPADD.mfb} ${DPADD.mi} ${LIBXEXT} ${DPADD.xkb} \
	${DPADD.lbx} ${LIBLBXUTIL} ${DPADD.dbe} ${DPADD.record} \
	${DPADD.xtrap} ${DPADD.GL/glx} ${DPADD.GL/GLcore} ${DPADD.randr} \
	${DPADD.render} ${LIBXFONT} ${LDADD.dix/xpstubs} ${LIBFREETYPE} \
	${LIBXAU} ${LIBXDMCP} ${LIBZ} ${LIBM}

SYMLINKS=	${X11BINDIR}/Xsun ${X11BINDIR}/X

.include <bsd.x11.mk>
.include <bsd.prog.mk>
