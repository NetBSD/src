#	$NetBSD: Makefile,v 1.2 2006/08/06 16:05:07 xtraeme Exp $
#
# Makefile for loadable AMD PowerNow K7 device driver kernel module
#

.include "../Makefile.inc"

.PATH:		$S/arch/x86/x86
.PATH:          $S/arch/i386/i386

CFILES=		lkminit_powernow.c powernow_common.c powernow_k7.c
SRCS=		${CFILES}

WARNS=		3

KMOD=		powernow
MAN=

CLEANFILES+=	*~ ${KMOD}

.include <bsd.kmod.mk>
