#	$NetBSD: Makefile,v 1.9 2003/10/19 04:45:01 lukem Exp $

PROG=	wsconsctl
SRCS=	display.c keyboard.c keysym.c map_parse.y map_scan.l \
		mouse.c util.c wsconsctl.c
MAN=	wsconsctl.8

YHEADER=	1
CPPFLAGS+=	-I. -I${.CURDIR}
DPSRCS+=	keysym.h
CLEANFILES+=	keysym.h

.include <bsd.prog.mk>

keysym.h: mkkeysym.sh ${DESTDIR}/usr/include/dev/wscons/wsksymdef.h
	${_MKMSGCREATE}
	${_MKCMD}\
	${HOST_SH} ${.CURDIR}/mkkeysym.sh \
	  ${DESTDIR}/usr/include/dev/wscons/wsksymdef.h > keysym.h
