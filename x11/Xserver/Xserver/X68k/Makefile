#	$NetBSD: Makefile,v 1.1 2004/01/07 13:02:54 minoura Exp $

.include <bsd.own.mk>

PROG=	X68k
BINMODE=4711

MAN=	X68k.1

FILES=	X68kConfig
FILESDIR=${X11LIBDIR}

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/x68k

.include "../Makefile.Xserver"

LDADD+=	${LDADD.dix} ${LDADD.mi} ${LDADD.os} -lXau -lXdmcp ${LDADD.hw/netbsd/x68k} \
	${LDADD.cfb16} ${LDADD.cfb} ${LDADD.mfb} \
	${LDADD.Xext/exts} ${LDADD.xkb} ${LDADD.xinput} ${LDADD.lbx} \
	-llbxutil ${LDADD.randr} ${LDADD.render} ${LDADD.dix/xpstubs} \
	${LDADD.mi} ${LDADD.Xext/exts} ${LDADD.xkb} ${LDADD.xinput} \
	${LDADD.lbx} -llbxutil ${LDADD.randr} ${LDADD.render} \
	${LDADD.hw/netbsd/x68k} ${LDADD.Xext/ext} ${LDADD.dbe} ${LDADD.record} \
	${LDADD.GL/glx} ${LDADD.GL/GLcore} ${LDADD.xtrap} ${LIBCWRAPPER} \
	-lXfont -lfreetype ${LDADD.dix/xpstubs} \
	-lXext -lX11 -lz -lm

DPADD+=	${DPADD.dix} ${DPADD.mi} ${DPADD.os} ${LIBXAU} ${LIBXDMCP} ${DPADD.hw/netbsd/x68k} \
	${DPADD.cfb16} ${DPADD.cfb} ${DPADD.mfb} \
	${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} ${DPADD.lbx} \
	${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} ${DPADD.dix/xpstubs} \
	${DPADD.mi} ${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} \
	${DPADD.lbx} ${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} \
	${DPADD.hw/netbsd/x68k} ${DPADD.Xext/ext} ${DPADD.dbe} ${DPADD.record} \
	${DPADD.GL/glx} ${DPADD.GL/GLcore} ${DPADD.xtrap} ${LIBCWRAPPER} \
	${LIBXFONT} ${LIBFREETYPE} ${DPADD.dix/xpstubs} \
	${LIBXEXT} ${LIBX11} ${LIBZ} ${LIBM}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
