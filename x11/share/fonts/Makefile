#	$NetBSD: Makefile,v 1.2 2003/09/13 00:19:57 lukem Exp $

.include <bsd.own.mk>

SUBDIR=	util encodings .WAIT
SUBDIR+=bdf local scaled

FCCACHEDIR!=	cd ${NETBSDSRCDIR}/x11/tools/fc-cache && ${PRINTOBJDIR}
FCCACHE=	${FCCACHEDIR}/fc-cache

afterinstall:
	${FCCACHE} -fv ${DESTDIR}${X11FONTDIR}
.if ${MKUNPRIVED} != "no"
	cd ${DESTDIR} \
	&& find ./${X11FONTDIR} -name fonts.cache-1 \
	    | sed -e 's/$$/ type=file mode=0444 uname=root gname=wheel/' \
	    | ${METALOG.add}
.endif

.include <bsd.subdir.mk>
