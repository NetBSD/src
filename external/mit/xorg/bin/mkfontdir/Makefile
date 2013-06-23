#	$NetBSD: Makefile,v 1.1.1.1.28.1 2013/06/23 06:28:39 tls Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.mkfontdir}
.PATH:	${X11SRCDIR.mkfontdir}/man

MAN=	mkfontdir.1

SCRIPTS=	mkfontdir

mkfontdir: mkfontdir.in
	${TOOL_SED} -e "s#@bindir@#${X11BINDIR}#" < ${.ALLSRC} > ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
