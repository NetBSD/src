#	$NetBSD: Makefile,v 1.7.60.1 2014/08/20 00:02:29 tls Exp $
#
#	@(#)Makefile	8.2 (Berkeley) 6/1/94

SECTION=reference/ref3
ARTICLE=sysman
SRCS=	0.t 1.0.t 1.1.t 1.2.t 1.3.t 1.4.t 1.5.t 1.6.t 1.7.t \
	2.0.t 2.1.t 2.2.t 2.3.t 2.4.t 2.5.t a.t
DEPSRCS=facilities contents
MACROS=	-ms
ROFF_TBL=yes
CLEANFILES+=facilities contents pagelog
EXTRAHTMLFILES=\
	sysman1.png sysman2.png sysman3.png sysman4.png sysman5.png \
	sysman6.png sysman7.png sysman8.png sysman9.png sysman10.png \
	sysman11.png sysman12.png sysman13.png sysman14.png sysman15.png \
	sysman16.png sysman17.png sysman18.png sysman19.png sysman20.png \
	sysman21.png sysman22.png sysman23.png sysman24.png sysman25.png \
	sysman26.png sysman27.png sysman28.png sysman29.png sysman30.png \
	sysman31.png sysman32.png sysman33.png

.include <bsd.doc.mk>

facilities: ${SRCS}
	${TOOL_SOELIM} -I${.CURDIR} ${.ALLSRC} |\
	    ${TOOL_SED} -n -e '/^\.Fd/p;/^\.Sh/p' |\
	    ${TOOL_SED} -e 's/^\.Fd/.Nm/;s/^\.Sh /.L/' > ${.TARGET}

contents: ${SRCS}
	${TOOL_SOELIM} -I${.CURDIR} ${.ALLSRC} |\
	    ${TOOL_SED} -n -e '/^\.Sh/p' |\
	    ${TOOL_SED} -e 's/^\.Sh /.L/;s/$$/"/' > ${.TARGET}
