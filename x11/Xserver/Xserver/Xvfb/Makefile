#	$NetBSD: Makefile,v 1.4.4.1 2004/11/15 09:12:38 rtr Exp $

.include <bsd.own.mk>

PROG=	Xvfb
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/vfb

.include "../Makefile.Xserver"

LDADD+=	${LDADD.dix} \
	${LDADD.hw/vfb} \
	${LDADD.os} \
	-lXau \
	-lXdmcp \
	${LDADD.fb} \
	${LDADD.mfb} \
	${LDADD.dix/xpstubs} \
	${LDADD.mi} \
	${LDADD.composite} \
	${LDADD.damage} \
	${LDADD.miext/damage} \
	${LDADD.xfixes} \
	${LDADD.miext/cw} \
	${LDADD.Xext/exts} \
	${LDADD.xkb} \
	${LDADD.xinput} \
	${LDADD.lbx} \
	-llbxutil \
	${LDADD.randr} \
	${LDADD.render} \
	${LDADD.mi} \
	${LDADD.Xext/ext} \
	${LDADD.dbe} \
	${LDADD.record} \
	${LDADD.GL/glx} \
	${LDADD.GL/GLcore} \
	${LDADD.xtrap} \
	${LIBCWRAPPER} \
	-lXfont \
	-lfreetype \
	${LDADD.dix/xpstubs} \
	-lz \
	-lm

DPADD+=	${DPADD.dix} ${DPADD.os} ${LIBXAU} ${LIBXDMCP} \
	${DPADD.hw/vfb} ${DPADD.fb} ${DPADD.mfb} ${DPADD.dix/xpstubs} \
	${DPADD.mi} ${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} \
	${DPADD.lbx} ${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} \
	${DPADD.mi} ${DPADD.Xext/ext} ${DPADD.dbe} ${DPADD.record} \
	${DPADD.GL/glx} ${DPADD.GL/GLcore} ${DPADD.xtrap} \
	${LIBCWRAPPER} \
	${LIBXFONT} ${LIBFREETYPE} ${DPADD.dix/xpstubs} \
	${LIBZ} ${LIBM}

.if ${MACHINE_ARCH} == "i386"
LDADD+=	-li386
DPADD+=	${LIBI386}
.endif

.include <bsd.x11.mk>
.include <bsd.prog.mk>
