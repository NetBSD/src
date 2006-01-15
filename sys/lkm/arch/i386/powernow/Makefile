#	$NetBSD: Makefile,v 1.1.2.2 2006/01/15 10:02:57 yamt Exp $
#
# Makefile for loadable AMD PowerNow K7 device driver kernel module
#

.include "../Makefile.inc"

.PATH:          $S/arch/i386/i386

CFILES=		lkminit_powernow.c powernow_k7.c
SRCS=		${CFILES}

WARNS=		3

KMOD=		powernow
MAN=

CLEANFILES+=	*~ ${KMOD}

.include <bsd.kmod.mk>
