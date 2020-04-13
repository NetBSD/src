# $NetBSD: Makefile,v 1.1.4.1 2020/04/13 08:03:03 martin Exp $

SRCS   = dhcrelay.c
PROG   = dhcrelay
MAN    = dhcrelay.8

DHCPSRCDIR=	relay

COPTS.dhcrelay.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
