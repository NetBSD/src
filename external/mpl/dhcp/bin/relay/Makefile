# $NetBSD: Makefile,v 1.3 2023/06/03 09:09:08 lukem Exp $

SRCS   = dhcrelay.c
PROG   = dhcrelay
MAN    = dhcrelay.8

DHCPSRCDIR=	relay

COPTS.dhcrelay.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
