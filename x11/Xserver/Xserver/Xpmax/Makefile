#	$NetBSD: Makefile,v 1.1 2004/01/10 05:26:14 rtr Exp $

.include <bsd.own.mk>

PROG=	Xpmax
BINMODE=4711

MKMAN=	no

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/pmax

.include "../Makefile.Xserver"

LDADD=	${LDADD.dix} ${LDADD.hw/netbsd/pmax} -lXfont -lfreetype \
	${LDADD.os} ${LDADD.cfb} ${LDADD.mfb} ${LDADD.xkb} ${LDADD.mi} \
	${LDADD.Xext/ext} ${LDADD.xinput} ${LDADD.lbx} -llbxutil \
	${LDADD.randr} ${LDADD.render} ${LDADD.dix/xpstubs} ${LDADD.dbe} \
	${LDADD.record} ${LDADD.GL/glx} ${LDADD.GL/GLcore} -lXau -lXdmcp \
	-lz -lm

DPADD=	${DPADD.dix} ${DPADD.hw/netbsd.pmax} ${LIBXFONT} ${LIBFREETYPE} \
	${DPADD.os} ${DPADD.cfb} ${DPADD.mfb} ${DPADD.xkb} ${DPADD.mi} \
	${DPADD.Xext/ext} ${DPADD.xinput} ${DPADD.lbx} ${LIBLBXUTIL} \
	${DPADD.randr} ${DPADD.render} ${DPADD.dix/xpstubs} ${DPADD.dbe} \
	${DPADD.record} ${DPADD.GL/glx} ${DPADD.GL/GLcore} ${LIBXAU} \
	${LIBXDMCP} ${LIBZ} ${LIBM}

SYMLINKS=	${X11BINDIR}/Xpmax ${X11BINDIR}/X

.include <bsd.x11.mk>
.include <bsd.prog.mk>
