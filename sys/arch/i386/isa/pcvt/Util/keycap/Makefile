LIB	=	keycap
CAPDIR	=	/usr/share/pcvt
CAPPATH	=	$(CAPDIR)/keycap.pcvt
KEYCAPSRC=	keycap.src
CPPFLAGS+=	-DKEYCAP_PATH=\"$(CAPPATH)\"
SRCS	=	keycap.c

MKMAN=	no

beforeinstall:
	@if [ ! -d ${DESTDIR}${CAPDIR} ]; then mkdir ${DESTDIR}${CAPDIR};fi
	@${ECHO} "Saving old keycap database"
	-mv -f ${DESTDIR}${CAPPATH} ${DESTDIR}${CAPPATH}.BAK
	${INSTALL} -c -m ${LIBMODE} -o ${LIBOWN} -g ${LIBGRP} \
	${.CURDIR}/${KEYCAPSRC} ${DESTDIR}${CAPPATH}

.include <bsd.lib.mk>
