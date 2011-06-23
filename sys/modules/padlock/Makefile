#	$NetBSD: Makefile,v 1.1.6.1 2011/06/23 14:20:25 cherry Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	padlock
IOCONF=	padlock.ioconf
SRCS=	via_padlock.c

WARNS=	4

COPTS.via_padlock.c+=	${${ACTIVE_CC} == "clang":?-no-integrated-as:}

.include <bsd.kmodule.mk>
