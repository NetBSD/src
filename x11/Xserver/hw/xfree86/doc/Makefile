#	$NetBSD: Makefile,v 1.1 2004/01/05 05:18:08 rtr Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/doc

CPPSCRIPTS=		BugReport
CPPSCRIPTFLAGS=		-DXF86_VERSION=4.3.0
SCRIPTS=		BugReport
SCRIPTSDIR_BugReport=	${X11USRLIBDIR}/X11/doc

FILES=	BUILD DESIGN Install LICENSE README README.DECtga README.DGA \
	README.DRI README.DRIcomp README.I128 README.SiS README.XKB-Config \
	README.XKB-Enhancing README.apm README.ati README.chips \
	README.cyrix README.dps README.fonts README.i740 README.i810 \
	README.mouse README.newport README.r128 README.rapidaccess \
	README.rendition README.s3virge RELNOTES ServersOnly Status \
	Versions VideoBoard98

FILESDIR=	${X11USRLIBDIR}/X11/doc
     
.include <bsd.x11.mk>
.include <bsd.prog.mk>
