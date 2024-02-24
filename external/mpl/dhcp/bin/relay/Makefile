# $NetBSD: Makefile,v 1.2.8.1 2024/02/24 13:06:55 martin Exp $

SRCS   = dhcrelay.c
PROG   = dhcrelay
MAN    = dhcrelay.8

DHCPSRCDIR=	relay

COPTS.dhcrelay.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
