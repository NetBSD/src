#	$NetBSD: Makefile,v 1.5.4.1 2009/05/13 19:20:46 jym Exp $

NOMAN=	# defined (must come before bsd.own.mk)

.include <bsd.own.mk>

PROG=	XsunMono
BINMODE=4711

.include "../Makefile.Xserver"

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/sun

LDADD=	${LIBDIR.hw/sun}/sunInitMono.o ${LIBDIR.hw/sun}/sunInExMono.o \
	${LDADD.hw/sun} ${LDADD.dix} ${LDADD.os} ${LDADD.mfb} ${LDADD.mi} \
	${LDADD.Xext/ext} ${LDADD.xkb}

.if ${MACHINE_ARCH} == "sparc64" || ${MACHINE_ARCH} == "sparc"
LDADD+= ${LDADD.xinput}
DPADD+=	${DPADD.xinput}
.endif

LDADD+=	${LDADD.lbx} -llbxutil ${LDADD.dbe} ${LDADD.record} ${LDADD.xtrap} \
	${LDADD.GL/glx} ${LDADD.GL/GLcore} ${LDADD.randr} ${LDADD.render} \
	-lXfont -lfreetype ${LDADD.dix/xpstubs} -lfreetype -lXau -lXdmcp -lz -lm

DPADD+=	${LIBDIR.hw/sun}/sunInitMono.o ${LIBDIR.hw/sun}/sunInExMono.o \
	${DPADD.hw/sun} ${DPADD.dix} ${DPADD.os} ${DPADD.mfb} ${DPADD.mi} \
	${LIBXEXT} ${DPADD.xkb} ${DPADD.lbx} ${LIBLBXUTIL} ${DPADD.dbe} \
	${DPADD.record} ${DPADD.xtrap} ${DPADD.GL/glx} ${DPADD.GL/GLcore} \
	${DPADD.randr} ${DPADD.render} ${LIBXFONT} ${DPADD.dix/xpstubs} \
	${LIBFREETYPE} ${LIBXAU} ${LIBXDMCP} ${LIBZ} ${LIBM}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
