#	$NetBSD: Makefile,v 1.3 2005/07/03 14:30:52 tron Exp $

.include <bsd.own.mk>

PROG=	Xamiga

MAN=	Xamiga.1
MLINKS=	Xamiga.1 Xamiga24.1

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/amiga

.include "../Makefile.Xserver"

LDADD=	${LDADD.dix} ${LDADD.hw/netbsd/amiga} ${LDADD.afb/afb} -lXfont \
	-lfreetype ${LDADD.os} ${LDADD.cfb} ${LDADD.cfb16} ${LDADD.cfb24} \
	${LDADD.cfb32} ${LDADD.mfb} ${LDADD.xkb} ${LDADD.mi} \
	${LDADD.Xext/ext} ${LDADD.xinput} ${LDADD.lbx} -llbxutil \
	${LDADD.randr} ${LDADD.render} ${LDADD.dix/xpstubs} \
	${LDADD.dbe} ${LDADD.record} ${LDADD.GL/glx} ${LDADD.GL/GLcore} \
	-lXau -lXdmcp -lz -lm

DPADD=	${DPADD.dix} ${DPADD.hw/netbsd/amiga} ${DPADD.afb/afb} ${LIBXFONT} \
	${LIBFREETYPE} ${DPADD.os} ${DPADD.cfb} ${DPADD.cfb16} \
	${DPADD.cfb24} ${DPADD.cfb32} ${DPADD.mfb} ${DPADD.xkb} \
	${DPADD.mi} ${DPADD.Xext/ext} ${DPADD.xinput} ${DPADD.lbx} \
	${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render} ${DPADD.dix/xpstubs} \
	${DPADD.dbe} ${DPADD.record} ${DPADD.GL/glx} ${DPADD.GL/GLcore} \
	${LIBXAU} ${LIBXDMCP} ${LIBZ} ${LIBM}

SYMLINKS=	${X11BINDIR}/Xamiga ${X11BINDIR}/X

.include <bsd.x11.mk>
.include <bsd.prog.mk>
