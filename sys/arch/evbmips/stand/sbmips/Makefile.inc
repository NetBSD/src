# $NetBSD: Makefile.inc,v 1.3.6.2 2017/12/03 11:36:10 jdolecek Exp $

.include <bsd.own.mk>

BINDIR?=		/usr/mdec/sbmips
WARNS?=			1

MKMAN=			no

CPPFLAGS+=		-I${.CURDIR}/../../..
LDSTATIC?=		-static
