#	$NetBSD: Makefile,v 1.2 2005/04/09 18:00:01 tron Exp $

.include <bsd.own.mk>

PROG=	Xdmx

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/dmx

.include "../Makefile.Xserver"

LDADD+= ${LDADD.dix} \
	${LDADD.hw/dmx} \
	${LDADD.os} \
	${LDADD.randr} \
	${LDADD.hw/dmx/dmxconfig} \
	${LDADD.miext/shadow} \
	${LDADD.fb} \
	${LDADD.mi} \
	${LDADD.Xext/ext} \
	${LDADD.record} \
	${LDADD.xtrap} \
	${LDADD.xkb} \
	${LDADD.xinput} \
	${LDADD.lbx} \
	-llbxutil \
	${LDADD.render} \
	${LDADD.hw/dmx/dmxinput} \
	-lXfont \
	-lfreetype \
	${LDADD.dix/xpstubs} \
	-lXi -lXmu -lXt -lSM \
	-lICE -lXext -lX11 \
	-lXrender -lXext -lX11 \
	-lz -lm -lXdmcp

DPADD+= ${DPADD.dix} \
	${DPADD.hw/dmx} \
	${DPADD.os} \
	${DPADD.randr} \
	${DPADD.hw/dmx/dmxconfig} \
	${DPADD.miext/shadow} \
	${DPADD.fb} \
	${DPADD.mi} \
	${DPADD.Xext/ext} \
	${DPADD.record} \
	${DPADD.xtrap} \
	${DPADD.xkb} \
	${DPADD.xinput} \
	${DPADD.lbx} \
	${DPADD.render} \
	${DPADD.hw/dmx/dmxinput} \
	${DPADD.dix/xpstubs}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
