#	$NetBSD: Makefile,v 1.5 2004/02/09 06:52:34 lukem Exp $

.include <bsd.own.mk>

SUBDIR=	util encodings .WAIT
SUBDIR+=bdf local scaled

.include "${NETBSDSRCDIR}/x11/tools/fc-cache/Makefile.fc-cache"

afterinstall:
	${_MKMSG_CREATE} ${DESTDIR}${X11FONTDIR}/fonts.cache-1
	${FCCACHE} -fv ${DESTDIR}${X11FONTDIR}
.if ${MKUNPRIVED} != "no"
	cd ${DESTDIR} \
	&& find ./${X11FONTDIR} -name fonts.cache-1 \
	    | sed -e 's/$$/ type=file mode=0444 uname=${BINOWN} gname=${BINGRP}/' \
	    | ${METALOG.add}
.endif

.include <bsd.subdir.mk>
