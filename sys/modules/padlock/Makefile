#	$NetBSD: Makefile,v 1.1.4.3 2011/06/12 00:24:30 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/x86

KMOD=	padlock
IOCONF=	padlock.ioconf
SRCS=	via_padlock.c

WARNS=	4

COPTS.via_padlock.c+=	${${ACTIVE_CC} == "clang":?-no-integrated-as:}

.include <bsd.kmodule.mk>
