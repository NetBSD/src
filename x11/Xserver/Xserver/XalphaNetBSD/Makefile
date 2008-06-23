#	$NetBSD: Makefile,v 1.1.34.1 2008/06/23 04:32:13 wrstuden Exp $

NOMAN=	# defined (must come before bsd.own.mk)

.include <bsd.own.mk>

PROG=	XalphaNetBSD
BINMODE=4711

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/alpha

.include "../Makefile.Xserver"

LDADD=	${LDADD.dix} ${LDADD.hw/netbsd/alpha} ${LDADD.os} -lXfont \
	-lfreetype ${LDADD.cfb} ${LDADD.cfb32} ${LDADD.mfb} ${LDADD.xkb} \
	${LDADD.mi} ${LDADD.Xext/ext} ${LDADD.lbx} \
	-llbxutil ${LDADD.randr} ${LDADD.render} ${LDADD.dix/xpstubs} \
	${LDADD.dbe} ${LDADD.record} ${LDADD.GL/glx} ${LDADD.GL/GLcore} \
	-lXau -lXdmcp -lz -lm

DPADD=	${DPADD.dix} ${DPADD.hw/netbsd/alpha} ${DPADD.os} \
	${LIBXFONT} ${LIBFREETYPE} ${DPADD.cfb} ${DPADD.cfb16} \
	${DPADD.mfb} ${DPADD.xkb} ${DPADD.mi} ${DPADD.Xext/ext} \
	${DPADD.lbx} ${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render \
	${DPADD.dix/xpstubs} ${DPADD.dbe} ${DPADD.record} \
	${DPADD.GL/glx} ${LDADD.GL/GLcore} ${LIBXAU} ${LIBXDMCP} \
	${LIBZ} ${LIBM}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
