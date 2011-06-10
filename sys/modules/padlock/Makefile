#	$NetBSD: Makefile,v 1.2 2011/06/10 04:45:41 joerg Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	padlock
IOCONF=	padlock.ioconf
SRCS=	via_padlock.c

WARNS=	4

COPTS.via_padlock.c+=	${${ACTIVE_CC} == "clang":?-no-integrated-as:}

.include <bsd.kmodule.mk>
