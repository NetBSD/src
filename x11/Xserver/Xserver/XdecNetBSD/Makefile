#	$NetBSD: Makefile,v 1.1 2004/01/18 05:35:04 rtr Exp $

.include <bsd.own.mk>

PROG=	XdecNetBSD
MKMAN=	no

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/dec

.include "../Makefile.Xserver"

LDADD=	${LDADD.dix} ${LDADD.hw/netbsd/dec} ${LDADD.os} -lXfont \
	-lfreetype ${LDADD.cfb} ${LDADD.cfb32} ${LDADD.mfb} ${LDADD.xkb} \
	${LDADD.mi} ${LDADD.Xext/ext} ${LDADD.xinput} ${LDADD.lbx} \
	-llbxutil ${LDADD.randr} ${LDADD.render} ${LDADD.dix/xpstubs} \
	${LDADD.dbe} ${LDADD.record} ${LDADD.GL/glx} \
	${LDADD.GL/GLcore} -lXau -lXdmcp -lz -lm

DPADD=	${DPADD.dix} ${DPADD.hw/netbsd/dec} ${DPADD.os} ${LIBXFONT} \
	${LIBFREETYPE} ${DPADD.cfb} ${DPADD.cfb32} ${DPADD.mfb} \
	${DPADD.xkb} ${DPADD.mi} ${DPADD.Xext/ext} ${DPADD.xinput} \
	${DPADD.lbx} ${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} \
	${DPADD.dix/xpstubs} ${DPADD.dbe} ${DPADD.record} \
	${DPADD.GL/glx} ${DPADD.GL/GLcore} ${LIBXAU} ${LIBXDMCP} \
	${LIBZ} ${LIBM}

SYMLINKS=	${X11BINDIR}/XdecNetBSD ${X11BINDIR}/X

.include <bsd.x11.mk>
.include <bsd.prog.mk>
