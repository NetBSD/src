#	$NetBSD: Makefile,v 1.1 1998/12/28 14:01:16 hannken Exp $

PROG=	wsconsctl
SRCS=	display.c keyboard.c keysym.c map_parse.y map_scan.l \
		mouse.c util.c wsconsctl.c
MAN=	wsconsctl.8

YHEADER=	1
CPPFLAGS+=	-I.
CLEANFILES+=	keysym.h

.depend keysym.c: keysym.h

keysym.h: mkkeysym.sh
	sh ${.CURDIR}/mkkeysym.sh \
	  ${DESTDIR}/usr/include/dev/wscons/wsksymdef.h > keysym.h

.include <bsd.prog.mk>
