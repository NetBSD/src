#	$NetBSD: Makefile,v 1.2 1999/01/10 18:05:49 augustss Exp $

PROG=	wsconsctl
SRCS=	display.c keyboard.c keysym.c map_parse.y map_scan.l \
		mouse.c util.c wsconsctl.c
MAN=	wsconsctl.8

YHEADER=	1
CPPFLAGS+=	-I.
CLEANFILES+=	keysym.h

.depend keysym.c: keysym.h

keysym.h: mkkeysym.sh ${DESTDIR}/usr/include/dev/wscons/wsksymdef.h
	sh ${.CURDIR}/mkkeysym.sh \
	  ${DESTDIR}/usr/include/dev/wscons/wsksymdef.h > keysym.h

.include <bsd.prog.mk>
