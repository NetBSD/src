#	$NetBSD: Makefile,v 1.1.2.2 2008/09/18 05:18:44 wrstuden Exp $

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
