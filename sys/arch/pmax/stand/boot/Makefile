#	$NetBSD: Makefile,v 1.5.2.1 2000/11/20 20:20:40 bouyer Exp $
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
