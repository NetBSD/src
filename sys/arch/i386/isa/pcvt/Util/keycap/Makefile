LIB	=	keycap
CAPDIR	=	/usr/share/misc
CAPPATH	=	$(CAPDIR)/keycap.pcvt
KEYCAPSRC=	keycap.src
CFLAGS	+=	-DKEYCAP_PATH=\"$(CAPPATH)\"
SRCS	=	keycap.c

NOMAN=	yes

beforeinstall:
	@if [ ! -d ${DESTDIR}${CAPDIR} ]; then mkdir ${DESTDIR}${CAPDIR};fi
	@${ECHO} "Saving old keycap database"
	-mv -f ${DESTDIR}${CAPPATH} ${DESTDIR}${CAPPATH}.BAK
	${INSTALL} -c -m ${LIBMODE} -o ${LIBOWN} -g ${LIBGRP} \
	${.CURDIR}/${KEYCAPSRC} ${DESTDIR}${CAPPATH}

.include <bsd.lib.mk>
