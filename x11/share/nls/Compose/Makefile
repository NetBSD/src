#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 03:06:13 lukem Exp $

.include <bsd.own.mk>

CTFILES=armscii-8 el_GR.UTF-8 en_US.UTF-8 georgian-academy georgian-ps \
	ibm-cp1133 iso8859-1 iso8859-2 iso8859-3 iso8859-4 iso8859-5 \
	iso8859-6 iso8859-7 iso8859-8 iso8859-9 iso8859-9e iso8859-10 \
	iso8859-13 iso8859-14 iso8859-15 ja ja.JIS ja.SJIS ko koi8-c koi8-r \
	koi8-u mulelao-1 vi_VN.tcvn vi_VN.viscii zh_CN zh_CN.gbk zh_TW \
	zh_TW.big5

FILESNAME=	Compose

.for _F in ${CTFILES}
${_F}.ct: ${_F}
	@rm -f ${.TARGET}
	${CPP} -undef -traditional \
	    < ${.ALLSRC} | ${X11TOOL_UNXCOMM} > ${.TARGET}

FILES+=			${_F}.ct
FILESDIR_${_F}.ct=	${X11LIBDIR}/locale/${_F}
CLEANFILES+=		${_F}.ct
.endfor

realall: ${FILES}

.PATH: ${X11SRCDIR.xc}/nls/Compose

.include <bsd.prog.mk>
.include <bsd.x11.mk>
