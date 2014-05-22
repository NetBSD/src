#	$NetBSD: Makefile,v 1.1.1.1.4.1 2014/05/22 14:09:36 yamt Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.mkfontdir}
.PATH:	${X11SRCDIR.mkfontdir}/man

MAN=	mkfontdir.1

SCRIPTS=	mkfontdir

mkfontdir: mkfontdir.in
	${TOOL_SED} -e "s#@bindir@#${X11BINDIR}#" < ${.ALLSRC} > ${.TARGET}

CLEANFILES+=	mkfontdir

.include <bsd.x11.mk>
.include <bsd.prog.mk>
