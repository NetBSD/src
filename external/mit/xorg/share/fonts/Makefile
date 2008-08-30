#	$NetBSD: Makefile,v 1.2 2008/08/30 07:15:17 rtr Exp $

.include <bsd.own.mk>

SUBDIR=	encodings .WAIT \
	font-alias font-util \
	100dpi 75dpi Type1 TTF misc

.include "${NETBSDSRCDIR}/external/mit/xorg/tools/fc-cache/Makefile.fc-cache"

.if 0
afterinstall:
	${_MKMSG_CREATE} ${DESTDIR}${X11FONTDIR}/fonts.cache-1
	${FCCACHE} -fv ${DESTDIR}${X11FONTDIR}
.if ${MKUNPRIVED} != "no"
	cd ${DESTDIR} \
	&& find ./${X11FONTDIR} -name fonts.cache-1 \
	    | sed -e 's/$$/ type=file mode=0444 uname=${BINOWN} gname=${BINGRP}/' \
	    | ${METALOG.add}
.endif
.endif

.include <bsd.subdir.mk>
