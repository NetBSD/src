#	$NetBSD: Makefile,v 1.2 2003/09/13 20:19:07 lukem Exp $

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

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
