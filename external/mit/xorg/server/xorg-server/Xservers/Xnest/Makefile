#	$NetBSD: Makefile,v 1.2 2008/08/03 07:32:08 mrg Exp $

.include <bsd.own.mk>

PROG=	Xnest

.PATH:	${X11SRCDIR.xorg-server}/hw/xnest

.include "../Makefile.Xserver"

LDADD+= \
	${LDADD.hw/xnest} \
	${LDADD.XORG_CORE} \
	${LDADD.mi} \
	${LDADD.config} \
	-lXfont \
	-lfreetype \
	-lXau \
	-lfontenc \
	-lpixman-1 \
	-lXdmcp \
	${LDADD.fb} \
	${LDADD.xfixes} \
	${LDADD.Xext/Xextbuiltin} \
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
	${LDADD.os}

.if 0
DPADD+=	${DPADD.dix} ${DPADD.os} ${LIBXAU} ${LIBXDMCP} ${DPADD.hw/xnest} \
	${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} ${DPADD.lbx} \
	${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} ${DPADD.dix/xpstubs} \
	${DPADD.mi} ${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} \
	${DPADD.lbx} ${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} \
	${DPADD.hw/xnest} ${DPADD.Xext/ext} ${DPADD.dbe} ${DPADD.record} \
	${DPADD.GL/glx} ${DPADD.GL/GLcore} ${DPADD.xtrap} ${LIBCWRAPPER} \
	${LIBXFONT} ${LIBFREETYPE} ${DPADD.dix/xpstubs} \
	${LIBXEXT} ${LIBX11} ${LIBZ} ${LIBM}
.endif

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
