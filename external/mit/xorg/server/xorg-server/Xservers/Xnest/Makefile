#	$NetBSD: Makefile,v 1.3 2008/08/03 08:10:31 mrg Exp $

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
	${LDADD.os} \
	-lXext \
	-lX11 \
	-lXau \
	-lfreetype

LDADD+= \
	${DPADD.hw/xnest} \
	${DPADD.XORG_CORE} \
	${DPADD.mi} \
	${DPADD.config} \
	${LIBXFONT} \
	${LIBXAU} \
	${LIBFONTENC} \
	${LIBPIXMAN-1} \
	${LIBXDMCP} \
	${DPADD.fb} \
	${DPADD.xfixes} \
	${DPADD.Xext/Xextbuiltin} \
	${DPADD.dbe} \
	${DPADD.XTrap} \
	${DPADD.record} \
	${DPADD.render} \
	${DPADD.randr} \
	${DPADD.damageext} \
	${DPADD.miext/damage} \
	${DPADD.miext/shadow} \
	${DPADD.Xi} \
	${DPADD.xkb} \
	${DPADD.composite} \
	${DPADD.dix/xpstubs} \
	${DPADD.os} \
	${LIBXEXT} \
	${LIBX11} \
	${LIBXAU} \
	${LIBFREETYPE}

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
