#	$NetBSD: Makefile,v 1.4 2002/01/30 20:32:29 tv Exp $

PROG=	wsconsctl
SRCS=	display.c keyboard.c keysym.c map_parse.y map_scan.l \
		mouse.c util.c wsconsctl.c
MAN=	wsconsctl.8

YHEADER=	1
CPPFLAGS+=	-I. -I${.CURDIR}
CLEANFILES+=	keysym.h

.depend keysym.c: keysym.h

.include <bsd.prog.mk>

keysym.h: mkkeysym.sh ${DESTDIR}/usr/include/dev/wscons/wsksymdef.h
	sh ${.CURDIR}/mkkeysym.sh \
	  ${DESTDIR}/usr/include/dev/wscons/wsksymdef.h > keysym.h
