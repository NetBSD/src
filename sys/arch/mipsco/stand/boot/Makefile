#	$NetBSD: Makefile,v 1.2 2000/09/26 09:11:04 wdk Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94

SECONDARY_PROG=	boot

SRCS=		start.S boot.c
SRCS+=		bootinfo.c callvec.c conf.c devopen.c saio.c
SRCS+=		prom.S

CPPFLAGS+=	-DBOOT_TYPE_NAME='"Secondary"'

afterinstall:
	cp -p ${DESTDIR}${BINDIR}/${PROG} ${DESTDIR}/

.include "../Makefile.booters"
