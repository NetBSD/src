#	$NetBSD: Makefile,v 1.2.6.1 2002/06/23 17:38:12 jdolecek Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94

SECONDARY_PROG=	boot

SRCS=		start.S boot.c
SRCS+=		bootinfo.c callvec.c conf.c devopen.c saio.c
SRCS+=		prom.S

CPPFLAGS+=	-DBOOT_TYPE_NAME='"Secondary"'

# Copy boot to / as well.
afterinstall:
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${STRIPFLAG} ${DESTDIR}${BINDIR}/${PROG} ${DESTDIR}/${PROG}

.include "../Makefile.booters"
