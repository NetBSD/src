#	$NetBSD: Makefile,v 1.3 1995/09/16 15:49:40 jtc Exp $

.if !exists(${.CURDIR}/../Makefile.inc)
error:
	@echo
	@echo "                      You MUST link/copy"
	@echo
	@echo "       ../Makefile.inc.NetBSD    or   ../Makefile.inc.FreeBSD"
	@echo
	@echo "             to ../Makefile.inc before you can proceed !"
	@echo
.else

LIB	=	keycap
CAPDIR	=	/usr/share/misc
CAPPATH	=	$(CAPDIR)/keycap.pcvt
KEYCAPSRC=	keycap.src
CFLAGS	+=	-DKEYCAP_PATH=\"$(CAPPATH)\"
SRCS	=	keycap.c
MAN	=	keycap.${MAN3EXT} keycap.${MAN5EXT}

beforeinstall:
	@if [ ! -d ${LIBDIR} ]; then mkdir ${LIBDIR};fi
	@echo "Saving old keycap database"
	-mv -f ${CAPPATH} ${CAPPATH}.BAK
	install -c -m ${LIBMODE} -o ${LIBOWN} -g ${LIBGRP} \
	${.CURDIR}/${KEYCAPSRC} ${CAPPATH}

.include <bsd.lib.mk>

.endif
