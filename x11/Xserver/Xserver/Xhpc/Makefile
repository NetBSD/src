#	$NetBSD: Makefile,v 1.2 2004/01/10 16:14:37 rtr Exp $

.include <bsd.own.mk>

PROG=	Xhpc
BINMODE=4711

MAN=	Xhpc.1

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/hpc

.include "../Makefile.Xserver"

LDADD+=	${LDADD.dix} ${LDADD.mi} ${LDADD.os} -lXau -lXdmcp \
	${LDADD.hw/netbsd/hpc} ${LDADD.cfb} ${LDADD.cfb16} ${LDADD.mfb} \
	${LDADD.Xext/exts} ${LDADD.xkb} ${LDADD.xinput} ${LDADD.lbx} \
	-llbxutil ${LDADD.randr} ${LDADD.render} ${LDADD.dix/xpstubs} \
	${LDADD.mi} ${LDADD.Xext/exts} ${LDADD.xkb} ${LDADD.xinput} \
	${LDADD.lbx} -llbxutil ${LDADD.randr} ${LDADD.render} \
	${LDADD.hw/netbsd/hpc} ${LDADD.Xext/ext} ${LDADD.dbe} \
	${LDADD.record} ${LDADD.GL/glx} ${LDADD.GL/GLcore} ${LDADD.xtrap} \
	${LIBCWRAPPER} -lXfont -lfreetype ${LDADD.dix/xpstubs} \
	-lXext -lX11 -lz -lm

DPADD+=	${DPADD.dix} ${DPADD.mi} ${DPADD.os} ${LIBXAU} ${LIBXDMCP} \
	${DPADD.hw/netbsd/hpc} ${DPADD.cfb} ${DPADD.cfb16} ${DPADD.mfb} \
	${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} ${DPADD.lbx} \
	${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} ${DPADD.dix/xpstubs} \
	${DPADD.mi} ${DPADD.Xext/exts} ${DPADD.xkb} ${DPADD.xinput} \
	${DPADD.lbx} ${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} \
	${DPADD.hw/netbsd/hpc} ${DPADD.Xext/ext} ${DPADD.dbe} \
	${DPADD.record} ${DPADD.GL/glx} ${DPADD.GL/GLcore} ${DPADD.xtrap} \
	${LIBCWRAPPER} ${LIBXFONT} ${LIBFREETYPE} ${DPADD.dix/xpstubs} \
	${LIBXEXT} ${LIBX11} ${LIBZ} ${LIBM}

SYMLINKS=	${X11BINDIR}/Xhpc ${X11BINDIR}/X

.include <bsd.x11.mk>
.include <bsd.prog.mk>
