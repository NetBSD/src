#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 03:09:57 lukem Exp $

.include <bsd.own.mk>

SUBDIR=	Compose XI18N_OBJS XLC_LOCALE


LTFILES=compose.dir locale.alias locale.dir

FILESDIR=	${X11LIBDIR}/locale

.for _F in ${LTFILES}
${_F}.lt: ${_F}
	@rm -f ${.TARGET}
	( \
	${CPP} -undef -traditional \
	    < ${.ALLSRC} | ${X11TOOL_UNXCOMM} \
	    | sed -e '/^[^#][^ 	]*:/s/://' -e '/^[^#].*[ 	].*:/d'; \
	${CPP} -undef -traditional \
	    < ${.ALLSRC} | ${X11TOOL_UNXCOMM} ; \
	) > ${.TARGET}

FILES+=			${_F}.lt
FILESNAME_${_F}.lt=	${_F}
CLEANFILES+=		${_F}.lt
.endfor

realall: ${FILES}


.PATH: ${X11SRCDIR.xc}/nls

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
.include <bsd.x11.mk>
