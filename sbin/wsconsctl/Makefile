#	$NetBSD: Makefile,v 1.12 2005/01/20 16:39:23 xtraeme Exp $

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
	${_MKTARGET_CREATE}
	${HOST_SH} ${.CURDIR}/mkkeysym.sh \
	  ${DESTDIR}/usr/include/dev/wscons/wsksymdef.h > keysym.h
