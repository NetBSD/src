#	$NetBSD: Makefile,v 1.1 2004/08/01 18:52:03 uch Exp $

.include <bsd.own.mk>

PROG=	Xnewsmips
BINMODE=4711

.include "../Makefile.Xserver"

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/newsmips

LDADD=	${LDADD.dix} ${LDADD.hw/netbsd/newsmips} ${LDADD.os} -lXfont \
	-lfreetype ${LDADD.os} ${LDADD.cfb} ${LDADD.cfb16} ${LDADD.cfb} \
	${LDADD.mfb} ${LDADD.dix/xpstubs} ${LDADD.mi} ${LDADD.Xext/ext} \
	${LDADD.xkb} ${LDADD.xinput} ${LDADD.lbx} -llbxutil \
	${LDADD.dbe} ${LDADD.record} ${LDADD.xtrap} ${LDADD.GL/glx} \
	${LDADD.GL/GLcore} ${LDADD.randr} ${LDADD.render} -lXau \
	-lXdmcp -lz -lm

DPADD=	${DPADD.dix} ${DPADD.hw/netbsd/newsmips} ${LIBXFONT} \
	${LIBFREETYPE} ${DPADD.os} ${DPADD.cfb} ${DPADD.cfb16} \
	${DPADD.xkb} ${DPADD.mfb} ${DPADD.dix/xpstubs} ${DPADD.mi} \
	${DPADD.Xext/ext} ${DPADD.xkb} ${DPADD.xinput} ${DPADD.lbx} \
	${LIBLBXUTIL} ${DPADD.dbe} ${DPADD.record} ${DPADD.xtrap} \
	${DPADD.GL/glx} ${DPADD.GL/GLcore} ${DPADD.randr} ${DPADD.render} \
	${LIBXAU} ${LIBXDMCP} ${LIBZ} ${LIBM}

SYMLINKS=	${X11BINDIR}/Xnewsmips ${X11BINDIR}/X

.include <bsd.x11.mk>
.include <bsd.prog.mk>
