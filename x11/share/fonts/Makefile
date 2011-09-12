#	$NetBSD: Makefile,v 1.7 2011/09/12 13:38:42 apb Exp $

.include <bsd.own.mk>

SUBDIR=	util encodings .WAIT
SUBDIR+=bdf local scaled

.include "${NETBSDSRCDIR}/x11/tools/fc-cache/Makefile.fc-cache"

afterinstall:
	${_MKMSG_CREATE} ${DESTDIR}${X11FONTDIR}/fonts.cache-1
	${FCCACHE} -fv ${DESTDIR}${X11FONTDIR}
.if ${MKUNPRIVED} != "no"
	cd ${DESTDIR} \
	&& find .${X11FONTDIR} -name fonts.cache-1 \
	    | ${TOOL_SED} -e \
		's/$$/ type=file mode=0444 uname=${BINOWN} gname=${BINGRP}/' \
	    | ${METALOG.add}
.endif

.include <bsd.subdir.mk>
