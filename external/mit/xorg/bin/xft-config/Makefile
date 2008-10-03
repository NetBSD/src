#	$NetBSD: Makefile,v 1.1 2008/10/03 03:52:15 cube Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.Xft} ${X11SRCDIR.Xft}/man

SCRIPTS=	xft-config
MAN=		xft-config.1

xft-config:	xft-config.in
	${_MKTARGET_CREATE}
	${TOOL_SED} \
		-e 's,@prefix@,${X11ROOTDIR},; \
		s,@exec_prefix@,$${prefix},; \
		s,@libdir@,$${prefix}/lib,; \
		s,@includedir@,$${prefix}/include,; \
		s,@VERSION@,2.1.12,; \
		s,@FREETYPE_LIBS@,-lfreetype -lz,; \
		s,@FREETYPE_CFLAGS@,, ; \
		s,@FONTCONFIG_LIBS@,-lfontconfig,; \
		s,@FONTCONFIG_CFLAGS@,,; \
		s,@XRENDER_LIBS@,-lXrender,; \
		s,@XRENDER_CFLAGS@,,; \
		s%-L\([^ 	]*\)%-Wl,-R\1 &%g' \
		< ${.ALLSRC} > ${.TARGET}.tmp && \
	mv -f ${.TARGET}.tmp ${.TARGET}

CLEANFILES+=	xft-config xft-config.tmp

.include <bsd.x11.mk>
.include <bsd.prog.mk>
