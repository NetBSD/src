#	$NetBSD: Makefile,v 1.1.32.1 2008/06/04 02:06:00 yamt Exp $

NOMAN=	# defined (must come before bsd.own.mk)

.include <bsd.own.mk>

PROG=	Xarm32VIDC
BINMODE=4711

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/arm32vidc

.include "../Makefile.Xserver"

LDADD=	${LDADD.dix} ${LDADD.hw/netbsd/arm32vidc} ${LDADD.os} -lXfont \
	-lfreetype ${LDADD.cfb} ${LDADD.cfb16} ${LDADD.mfb} ${LDADD.xkb} \
	${LDADD.mi} ${LDADD.Xext/ext} ${LDADD.lbx} \
	-llbxutil ${LDADD.randr} ${LDADD.render} ${LDADD.dix/xpstubs} \
	${LDADD.dbe} ${LDADD.record} ${LDADD.GL/glx} ${LDADD.GL/GLcore} \
	-lXau -lXdmcp -lz -lm

DPADD=	${DPADD.dix} ${DPADD.hw/netbsd/arm32vidc} ${DPADD.os} \
	${LIBXFONT} ${LIBFREETYPE} ${DPADD.cfb} ${DPADD.cfb16} \
	${DPADD.mfb} ${DPADD.xkb} ${DPADD.mi} ${DPADD.Xext/ext} \
	${DPADD.lbx} ${LIBLBXUTIL} ${DPADD.randr} ${DPADD.render \
	${DPADD.dix/xpstubs} ${DPADD.dbe} ${DPADD.record} \
	${DPADD.GL/glx} ${LDADD.GL/GLcore} ${LIBXAU} ${LIBXDMCP} \
	${LIBZ} ${LIBM}

SYMLINKS=	${X11BINDIR}/Xarm32VIDC ${X11BINDIR}/X

.include <bsd.x11.mk>
.include <bsd.prog.mk>
