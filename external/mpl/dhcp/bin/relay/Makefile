# $NetBSD: Makefile,v 1.2 2019/10/13 07:28:09 mrg Exp $

SRCS   = dhcrelay.c
PROG   = dhcrelay
MAN    = dhcrelay.8

DHCPSRCDIR=	relay

COPTS.dhcrelay.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
