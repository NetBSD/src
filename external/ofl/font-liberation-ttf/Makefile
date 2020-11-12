#	$NetBSD: Makefile,v 1.1 2020/11/12 22:28:02 maya Exp $

.include <bsd.own.mk>

FONTSUBDIR=	TTF
FONTSRCPKG=	font-liberation-ttf
FILESDIR=	${X11FONTDIR}/TTF

.PATH:	${X11SRCDIR}/external/ofl/${FONTSRCPKG}/src \
	${X11SRCDIR}/external/ofl/${FONTSRCPKG}/dist

FILES=		LiberationMono-Bold.ttf LiberationMono-BoldItalic.ttf \
		LiberationMono-Italic.ttf LiberationMono-Regular.ttf \
		LiberationSans-Bold.ttf LiberationSans-BoldItalic.ttf \
		LiberationSans-Italic.ttf LiberationSans-Regular.ttf \
		LiberationSerif-Bold.ttf LiberationSerif-BoldItalic.ttf \
		LiberationSerif-Italic.ttf LiberationSerif-Regular.ttf

.include <bsd.x11.mk>
.include <bsd.files.mk>
.include <bsd.prog.mk>
