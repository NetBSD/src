#	$NetBSD: Makefile,v 1.17 2021/12/25 13:54:13 mlelstv Exp $

PROG=	wsconsctl
SRCS=	display.c keyboard.c keysym.c map_parse.y map_scan.l \
		mouse.c util.c wsconsctl.c edid.c vesagtf.o videomode.o
MAN=	wsconsctl.8

YHEADER=	1
CPPFLAGS+=	-I. -I${.CURDIR} -I${NETBSDSRCDIR}/sys
DPSRCS+=	keysym.h
CLEANFILES+=	keysym.h

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/sys/dev/videomode

# Environment for scripts executed during build.
SCRIPT_ENV= \
	AWK=${TOOL_AWK:Q} \
	SED=${TOOL_SED:Q}

keysym.h: mkkeysym.sh ${DESTDIR}/usr/include/dev/wscons/wsksymdef.h
	${_MKTARGET_CREATE}
	${SCRIPT_ENV} ${HOST_SH} ${.CURDIR}/mkkeysym.sh \
	  ${DESTDIR}/usr/include/dev/wscons/wsksymdef.h > keysym.h
