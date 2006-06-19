# $NetBSD: Makefile,v 1.1 2006/06/19 15:44:36 gdamore Exp $

LIB=	bluetooth

SRCS=	bluetooth.c devaddr.c lexer.l parser.y
MAN=	bluetooth.3 bluetooth.conf.5

CPPFLAGS+=	-I${.CURDIR}

LPREFIX=	_btyy
YPREFIX=	_btyy
YHEADER=	1

INCS=		bluetooth.h
INCSDIR=	/usr/include

MLINKS+=	bluetooth.3 bt_gethostbyname.3
MLINKS+=	bluetooth.3 bt_gethostbyaddr.3
MLINKS+=	bluetooth.3 bt_gethostent.3
MLINKS+=	bluetooth.3 bt_sethostent.3
MLINKS+=	bluetooth.3 bt_endhostent.3

MLINKS+=	bluetooth.3 bt_getprotobyname.3
MLINKS+=	bluetooth.3 bt_getprotobynumber.3
MLINKS+=	bluetooth.3 bt_getprotoent.3
MLINKS+=	bluetooth.3 bt_setprotoent.3
MLINKS+=	bluetooth.3 bt_endprotoent.3

MLINKS+=	bluetooth.3 bt_ntoa.3
MLINKS+=	bluetooth.3 bt_aton.3

MLINKS+=	bluetooth.3 bt_devaddr.3
MLINKS+=	bluetooth.3 bt_devname.3

MLINKS+=	bluetooth.3 bt_openconfig.3
MLINKS+=	bluetooth.3 bt_getconfig.3
MLINKS+=	bluetooth.3 bt_freeconfig.3
MLINKS+=	bluetooth.3 bt_eachconfig.3
MLINKS+=	bluetooth.3 bt_closeconfig.3
MLINKS+=	bluetooth.3 bt_printconfig.3

.include <bsd.lib.mk>
