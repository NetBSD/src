#	$NetBSD: Makefile,v 1.6.8.1 2002/02/11 20:08:51 jdolecek Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94

SECONDARY_PROG=	boot.pmax

SRCS=		start.S boot.c
SRCS+=		bootinfo.c callvec.c conf.c devopen.c putchar.c rz.c
SRCS+=		bootinit.S bootread.S getchar.S startprog.S

CPPFLAGS+=	-DBOOT_TYPE_NAME='"Secondary"'

# Copy bootpmax to / as well.
afterinstall:
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${STRIPFLAG} ${DESTDIR}${BINDIR}/${PROG} ${DESTDIR}/${PROG}

.include "../Makefile.booters"
