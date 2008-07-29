#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:23 mrg Exp $

.include <bsd.own.mk>

SUBDIR=	encodings .WAIT \
	font-alias font-util \
	100dpi 75dpi Type1 TTF misc

.include "${NETBSDSRCDIR}/external/mit/xorg/tools/fc-cache/Makefile.fc-cache"

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
