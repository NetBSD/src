#	$NetBSD: Makefile,v 1.2.6.2 2008/10/19 22:41:05 haad Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xextproto}

INCS=	MITMisc.h XEVI.h XEVIstr.h XLbx.h XShm.h XTest.h Xag.h Xagstr.h \
	Xcup.h Xcupstr.h Xdbe.h Xdbeproto.h Xext.h Xge.h dpms.h dpmsstr.h \
	extutil.h ge.h geproto.h lbxbuf.h lbxbufstr.h lbxdeltastr.h \
	lbximage.h lbxopts.h lbxstr.h lbxzlib.h mitmiscstr.h multibuf.h \
	multibufst.h security.h securstr.h shape.h shapestr.h shmstr.h sync.h \
	syncstr.h xtestext1.h xteststr.h
INCSDIR=${X11INCDIR}/X11/extensions

NOOBJ=	# defined

PKGCONFIG=	xextproto

.include <bsd.x11.mk>
.include <bsd.prog.mk>
