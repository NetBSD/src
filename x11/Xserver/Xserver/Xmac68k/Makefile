#	$NetBSD: Makefile,v 1.2 2004/01/10 14:44:18 rtr Exp $

.include <bsd.own.mk>

PROG=	Xmac68k
BINMODE=4711

MAN=	Xmac68k.1

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/mac68k

.include "../Makefile.Xserver"

LDADD+=	${LDADD.dix} ${LDADD.mi} ${LDADD.os} -lXau -lXdmcp \
	${LDADD.hw/netbsd/mac68k} ${LDADD.cfb} ${LDADD.mfb} \
	${LDADD.Xext/exts} ${LDADD.xkb} ${LDADD.xinput} ${LDADD.lbx} \
	-llbxutil ${LDADD.randr} ${LDADD.render} ${LDADD.dix/xpstubs} \
	${LDADD.mi} ${LDADD.Xext/exts} ${LDADD.xkb} ${LDADD.xinput} \
	${LDADD.lbx} -llbxutil ${LDADD.randr} ${LDADD.render} \
	${LDADD.hw/netbsd/mac68k} ${LDADD.Xext/ext} ${LDADD.dbe} \
	${LDADD.record} ${LDADD.GL/glx} ${LDADD.GL/GLcore} ${LDADD.xtrap} \
	${LIBCWRAPPER} -lXfont -lfreetype ${LDADD.dix/xpstubs} \
	-lXext -lX11 -lz -lm

DPADD+=	${DPADD.dix} ${DPADD.mi} ${DPADD.os} ${LIBXAU} ${LIBXDMCP} \
	${DPADD.hw/netbsd/mac68k} ${DPADD.cfb} ${DPADD.mfb} \
	${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} ${DPADD.lbx} \
	${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} ${DPADD.dix/xpstubs} \
	${DPADD.mi} ${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} \
	${DPADD.lbx} ${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} \
	${DPADD.hw/netbsd/mac68k} ${DPADD.Xext/ext} ${DPADD.dbe} \
	${DPADD.record} ${DPADD.GL/glx} ${DPADD.GL/GLcore} ${DPADD.xtrap} \
	${LIBCWRAPPER} ${LIBXFONT} ${LIBFREETYPE} ${DPADD.dix/xpstubs} \
	${LIBXEXT} ${LIBX11} ${LIBZ} ${LIBM}

SYMLINKS=	${X11BINDIR}/Xmac68k ${X11BINDIR}/X

.include <bsd.x11.mk>
.include <bsd.prog.mk>
