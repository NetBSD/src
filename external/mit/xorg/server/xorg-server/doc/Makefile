#	$NetBSD: Makefile,v 1.1 2008/08/02 04:32:00 mrg Exp $

.include <bsd.own.mk>

SecurityPolicy.5: SecurityPolicy.man.pre
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${CPP} -undef -traditional \
		-D__default_font_path__="`echo ${COMPILEDDEFAULTFONTPATH} | sed -e 's/,/, /g'`" \
		< ${.ALLSRC} \
		> ${.TARGET}

Xserver.1: Xserver.man.pre
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${CPP} -undef -traditional \
		-D__default_font_path__="`echo ${COMPILEDDEFAULTFONTPATH} | sed -e 's/,/, /g'`" \
		< ${.ALLSRC} \
		> ${.TARGET}

MAN=	SecurityPolicy.5 Xserver.1

.PATH:	${X11SRCDIR.xorg-server}/doc

.include <bsd.x11.mk>
.include <bsd.man.mk>
