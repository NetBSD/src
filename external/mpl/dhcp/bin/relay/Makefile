# $NetBSD: Makefile,v 1.2.8.2 2024/02/26 10:31:27 martin Exp $

SRCS   = dhcrelay.c
PROG   = dhcrelay
MAN    = dhcrelay.8

DHCPSRCDIR=	relay

COPTS.dhcrelay.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
