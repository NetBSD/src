#	$NetBSD: Makefile,v 1.3 2006/08/27 10:10:55 xtraeme Exp $
#
# Makefile for loadable AMD PowerNow K[78] device driver kernel module
#

.include "../Makefile.inc"

.PATH:		$S/arch/x86/x86
.PATH:          $S/arch/i386/i386

CFILES= 	lkminit_powernow.c powernow_common.c powernow_k7.c \
 		powernow_k8.c
SRCS=		${CFILES}

WARNS=		3

KMOD=		powernow
MAN=

# Enable this one to show debug output
# CPPFLAGS+=	-DPOWERNOW_DEBUG

CLEANFILES+=	*~ ${KMOD}

.include <bsd.kmod.mk>
