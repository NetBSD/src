#       $NetBSD: Makefile,v 1.2 1996/01/09 01:07:35 thorpej Exp $

#	Makefile for section 9 (kernel function and variable) manual pages.

MAN=	boot.9 disk.9 doshutdownhooks.9 inittodr.9 resettodr.9 \
	shutdownhook_establish.9 time.9
MLINKS+=shutdownhook_establish.9 shutdownhook_disestablish.9
MLINKS+=time.9 boottime.9 time.9 mono_time.9 time.9 runtime.9
MLINKS+=disk.9 disk_init.9 disk.9 disk_attach.9 disk.9 disk_detatch.9 \
	disk.9 disk_busy.9 disk.9 disk_unbusy.9 disk.9 disk_find.9 \
	disk.9 disk_resetstat.9

.include <bsd.prog.mk>
