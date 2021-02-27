#	$NetBSD: Makefile,v 1.3 2021/02/27 16:08:08 christos Exp $

.include <bsd.own.mk>

WARNS=	5
PROG=	resize
SRCS=	resize.c

CPPFLAGS+=-DRESIZE_ONLY -Wno-format-nonliteral
DPADD+=	${LIBUTIL}
LDADD+= -lutil

.include <bsd.prog.mk>

XTERM=${X11SRCDIR}/external/mit/xterm/dist

.for i in resize.c resize.man
copy:: ${.CURDIR}/${i}
${.CURDIR}/${i}: ${XTERM}/${i}
	cp -p ${.ALLSRC} ${.TARGET}
.endfor

CLEANFILES+=resize.1

resize.1: resize.man
	@rm -f ${.TARGET}
	${TOOL_SED} \
	    -e "s@__app_date__@1970-01-01@" \
	    -e "s@__app_version__@NetBSD@" \
	    -e "s@__default_termname__@vt100@" \
	    -e "s@__mansuffix__@1@" \
	    -e "s@__miscmansuffix__@7@" \
	    < ${.ALLSRC} > ${.TARGET}
