# $NetBSD: Makefile,v 1.1 2005/03/22 21:43:24 rtr Exp $

.include <bsd.own.mk>

SCRIPTSDIR=	${X11BINDIR}
FILESDIR=	${X11LIBDIR}/getconfig

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/getconfig
SCRIPTS=	getconfig.sh getconfig.pl
FILES=		cfg.sample xfree86.cfg

SCRIPTSNAME_getconfig.sh=	getconfig
SCRIPTSNAME_getconfig.pl=	getconfig.pl

.include <bsd.prog.mk>
.include <bsd.x11.mk>
