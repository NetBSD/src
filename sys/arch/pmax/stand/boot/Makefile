#	$NetBSD: Makefile,v 1.5.8.1 1999/12/27 18:33:30 wrstuden Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94

SECONDARY_PROG=	boot.pmax

SRCS=		start.S boot.c
SRCS+=		bootinfo.c callvec.c conf.c devopen.c putchar.c rz.c
SRCS+=		bootinit.S bootread.S getchar.S startprog.S

CPPFLAGS+=	-DBOOT_TYPE_NAME='"Secondary"'

# Copy bootpmax to / as well.
afterinstall:
	cp -p ${DESTDIR}${BINDIR}/${PROG} ${DESTDIR}/

.include "../Makefile.booters"
