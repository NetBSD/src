# $NetBSD: Makefile,v 1.1.6.1 2024/02/29 11:38:37 martin Exp $

SRCS   = dhcrelay.c
PROG   = dhcrelay
MAN    = dhcrelay.8

DHCPSRCDIR=	relay

COPTS.dhcrelay.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
