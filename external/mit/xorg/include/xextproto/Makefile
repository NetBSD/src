#	$NetBSD: Makefile,v 1.4 2010/05/22 05:44:08 mrg Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xextproto}

INCS=	EVI.h \
	EVIproto.h \
	ag.h \
	agproto.h \
	cup.h \
	cupproto.h \
	dbe.h \
	dbeproto.h \
	dpmsconst.h \
	dpmsproto.h \
	ge.h \
	geproto.h \
	lbx.h \
	lbxproto.h \
	mitmiscconst.h \
	mitmiscproto.h \
	multibufconst.h \
	multibufproto.h \
	secur.h \
	securproto.h \
	shapeconst.h \
	shapeproto.h \
	shm.h \
	shmproto.h \
	syncconst.h \
	syncproto.h \
	xtestconst.h \
	xtestext1const.h \
	xtestext1proto.h \
	xtestproto.h
# XXX XXX XXX xorg-server 1.7 won't need this
INCS+=	shmstr.h
#
INCSDIR=${X11INCDIR}/X11/extensions

NOOBJ=	# defined

PKGCONFIG=	xextproto

.include <bsd.x11.mk>
.include <bsd.prog.mk>
