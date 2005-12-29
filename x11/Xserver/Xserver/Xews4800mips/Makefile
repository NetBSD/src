#	$NetBSD: Makefile,v 1.1 2005/12/29 15:52:20 tsutsui Exp $

.include <bsd.own.mk>

PROG=	Xews4800mips
BINMODE=4711

.include "../Makefile.Xserver"

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/ews4800mips

LDADD=	${LDADD.dix} ${LDADD.hw/netbsd/ews4800mips} ${LDADD.os} -lXfont \
	-lfreetype ${LDADD.os} ${LDADD.cfb} ${LDADD.cfb16} ${LDADD.cfb} \
	${LDADD.mfb} ${LDADD.dix/xpstubs} ${LDADD.mi} ${LDADD.Xext/ext} \
	${LDADD.xkb} ${LDADD.xinput} ${LDADD.lbx} -llbxutil \
	${LDADD.dbe} ${LDADD.record} ${LDADD.xtrap} ${LDADD.GL/glx} \
	${LDADD.GL/GLcore} ${LDADD.randr} ${LDADD.render} -lXau \
	-lXdmcp -lz -lm

DPADD=	${DPADD.dix} ${DPADD.hw/netbsd/ews4800mips} ${LIBXFONT} \
	${LIBFREETYPE} ${DPADD.os} ${DPADD.cfb} ${DPADD.cfb16} \
	${DPADD.xkb} ${DPADD.mfb} ${DPADD.dix/xpstubs} ${DPADD.mi} \
	${DPADD.Xext/ext} ${DPADD.xkb} ${DPADD.xinput} ${DPADD.lbx} \
	${LIBLBXUTIL} ${DPADD.dbe} ${DPADD.record} ${DPADD.xtrap} \
	${DPADD.GL/glx} ${DPADD.GL/GLcore} ${DPADD.randr} ${DPADD.render} \
	${LIBXAU} ${LIBXDMCP} ${LIBZ} ${LIBM}

SYMLINKS=	${X11BINDIR}/Xews4800mips ${X11BINDIR}/X

.include <bsd.x11.mk>
.include <bsd.prog.mk>
