#	$NetBSD: Makefile,v 1.1 2004/01/17 04:03:26 rtr Exp $

.include <bsd.own.mk>

PROG=	Xamiga

MAN=	Xamiga.1
MLINKS=	Xamiga.1 Xamiga24.1

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/amiga

.include "../Makefile.Xserver"

LDADD=	${LDADD.dix} ${LDADD.hw/netbsd/amiga} ${LDADD.afb} -lXfont \
	-lfreetype ${LDADD.os} ${LDADD.cfb} ${LDADD.cfb16} ${LDADD.cfb24} \
	${LDADD.cfb32} ${LDADD.mfb} ${LDADD.xkb} ${LDADD.mi} \
	${LDADD.Xext/ext} ${LDADD.xinput} ${LDADD.lbx} -llbxutil \
	${LDADD.randr} ${LDADD.render} ${LDADD.dix/xpstubs} \
	${LDADD.dbe} ${LDADD.record} ${LDADD.GL/glx} ${LDADD.GL/GLcore} \
	-lXau -lXdmcp -lz -lm

SYMLINKS=	${X11BINDIR}/Xamiga ${X11BINDIR}/X

.include <bsd.x11.mk>
.include <bsd.prog.mk>
